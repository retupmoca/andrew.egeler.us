#include <string>
#include <string_view>
#include <memory>
#include <map>

#include <restinio/all.hpp>
#include <ctemplate/template.h>

#include <cmark.h>
#include <glob.h>

using router_t = restinio::router::express_router_t<>;
using std::vector;
using std::string;

extern "C" {
    extern char const template_layout[];
    extern unsigned const template_layout_size;

    extern char const template_index[];
    extern unsigned const template_index_size;

    extern char const static_style[];
    extern unsigned const static_style_size;

    extern char const static_favicon[];
    extern unsigned const static_favicon_size;
}

struct PostFile {
    string name;
    string filename;
    string prev = "";
    string next = "";
};

struct PostLoaded {
    string name;
    string htmlBody;
    string prev;
    string next;
};

struct PostRendered {
    string name;
    string html;
};

using PostMap = std::map<string, string>;

auto findPostFiles(string basePath) -> vector<PostFile> {
    // TODO: possible leak, because C

    vector<PostFile> posts;

    string last;

    glob_t globbuf;
    glob(basePath.c_str(), 0, nullptr, &globbuf);
    for(size_t i=0; i < globbuf.gl_pathc; i++) {
        // we assume that glob pulls files in lexographic order
        string filename(globbuf.gl_pathv[i]);
        string name = filename.substr(5, filename.size() - 8); // TODO: magic numbers, fragile

        posts.push_back({.name=name, .filename=filename});

        if (last.size()) {
            posts[posts.size() - 1].prev = last;
            posts[posts.size() - 2].next = name;
        }
        last = name;
    }
    globfree(&globbuf);

    return posts;
}

auto makePosts(string basePath) -> PostMap {
    // glob the files
    auto postFiles = findPostFiles(basePath);

    // render the markdown to HTML
    vector<PostLoaded> postPieces;
    std::transform(postFiles.begin(), postFiles.end(), std::back_inserter(postPieces),
        [](PostFile p) -> PostLoaded {
            // TODO: this should be a bit more C++ - fstream instead of FILE*
            //       at the very least
            cmark_parser *parser = cmark_parser_new(CMARK_OPT_DEFAULT);
            FILE *fp = fopen(p.filename.c_str(), "rb");
            size_t bytes;
            char buffer[8192];
            while ((bytes = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
                cmark_parser_feed(parser, buffer, bytes);
                if (bytes < sizeof(buffer)) {
                    break;
                }
            }
            char *document = cmark_render_html(cmark_parser_finish(parser), 0);
            string html(document);
            free(document);
            cmark_parser_free(parser);

            return {.name=p.name, .htmlBody=html, .prev=p.prev, .next=p.next};
        }
    );

    // dump it into HTML templates
    vector<PostRendered> postRendered;
    std::transform(postPieces.begin(), postPieces.end(), std::back_inserter(postRendered),
        [](PostLoaded p) -> PostRendered {
            ctemplate::TemplateDictionary dict("");
            dict.SetValue("post", p.htmlBody);
            dict.SetValue("next_link", p.next);
            if (!p.next.size())
                dict.ShowSection("hnext");
            dict.SetValue("prev_link", p.prev);
            if (!p.prev.size())
                dict.ShowSection("hprev");
            std::string output;
            ctemplate::ExpandTemplate("index.html", ctemplate::DO_NOT_STRIP, &dict, &output);

            return {.name=p.name, .html=output};
        }
    );

    // index it
    PostMap posts;
    for(auto &p : postRendered)
        posts[p.name] = p.html;
    return posts;
}

int main() {
    ctemplate::StringToTemplateCache("layout.html", template_layout, template_layout_size, ctemplate::DO_NOT_STRIP);
    ctemplate::StringToTemplateCache("index.html", template_index, template_index_size, ctemplate::DO_NOT_STRIP);

    PostMap posts = makePosts("post/*.md");
    string home_redirect = "/post/" + posts.rbegin()->first;

    auto router = std::make_unique<router_t>();

    router->http_get("/post/:pid", [&posts](auto req, auto params){
        try {
            auto name = restinio::cast_to<std::string>(params["pid"]);
            auto html = posts.at(name);

            req->create_response()
                .append_header(restinio::http_field::content_type, "text/html")
                .set_body(html)
                .done();
        }
        catch (const std::out_of_range& oor) {
            req->create_response( restinio::status_not_found() ).done();
        }
        catch(std::exception const & ex) {
            std::cout << ex.what() << std::endl;
            req->create_response( restinio::status_internal_server_error() ).done();
        }
        catch(...) {
            std::cout << "ERR" << std::endl;
            req->create_response( restinio::status_internal_server_error() ).done();
        }
        
        return restinio::request_accepted();
    });

    router->http_get("/favicon.ico", [](auto req, [[maybe_unused]] auto params){
        req->create_response()
            .append_header(restinio::http_field::content_type, "image/x-icon")
            .set_body(std::string_view(static_favicon, static_favicon_size))
            .done();

        return restinio::request_accepted();
    });

    router->http_get("/style.css", [](auto req, [[maybe_unused]] auto params){
        req->create_response()
            .append_header(restinio::http_field::content_type, "text/css")
            .set_body(std::string_view(static_style, static_style_size))
            .done();
        return restinio::request_accepted();
    });

    router->http_get("/", [&home_redirect](auto req, [[maybe_unused]] auto params){
        try {
            req->create_response(restinio::status_found())
                .append_header(restinio::http_field::location, home_redirect)
                .done();
        }
        catch (const std::out_of_range& oor) {
            req->create_response( restinio::status_not_found() ).done();
        }
        catch(...) {
            std::cout << "ERR" << std::endl;
            req->create_response( restinio::status_internal_server_error() ).done();
        }
        return restinio::request_accepted();
    });

    using traits_t =
      restinio::traits_t<
         restinio::asio_timer_manager_t,
         restinio::single_threaded_ostream_logger_t,
         router_t >;
    std::cout << "Starting server." << std::endl;
    restinio::run(restinio::on_this_thread<traits_t>().port(8080).request_handler(std::move(router)));

    return 0;
}

