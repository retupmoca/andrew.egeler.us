#include <string>
#include <string_view>
#include <memory>
#include <map>

#include <restinio/all.hpp>
#include <ctemplate/template.h>

#include <cmark.h>
#include <glob.h>

using router_t = restinio::router::express_router_t<>;
using std::shared_ptr;

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

std::map<std::string, char*> post_data;
std::map<std::string, std::string> prev_links;
std::map<std::string, std::string> next_links;

int main() {
    std::string last;
    // load blog posts
    glob_t globbuf;
    glob("post/*.md", 0, nullptr, &globbuf);
    for(size_t i=0; i < globbuf.gl_pathc; i++) {
        std::cout << globbuf.gl_pathv[i] << std::endl;

        cmark_parser *parser = cmark_parser_new(CMARK_OPT_DEFAULT);
        FILE *fp = fopen(globbuf.gl_pathv[i], "rb");
        size_t bytes;
        char buffer[8192];
        while ((bytes = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
            cmark_parser_feed(parser, buffer, bytes);
            if (bytes < sizeof(buffer)) {
                break;
            }
        }
        char *document = cmark_render_html(cmark_parser_finish(parser), 0);
        cmark_parser_free(parser);

        auto name = std::string(globbuf.gl_pathv[i]);
        name = name.substr(5, name.size() - 8);

        prev_links.insert_or_assign(name, "");
        next_links.insert_or_assign(name, "");

        //std::cout << name.substr(5, name.size() - 8) << std::endl;
        post_data.insert_or_assign(name, document);
        //std::cout << document << std::endl;
        if (last.size()) {
            prev_links.insert_or_assign(name, last);
            next_links.insert_or_assign(last, name);
        }
        last = name;
    }

    std::string home_redirect = "/post/" + std::string(post_data.rbegin()->first);

    globfree(&globbuf);

    ctemplate::StringToTemplateCache("layout.html", template_layout, template_layout_size, ctemplate::DO_NOT_STRIP);
    ctemplate::StringToTemplateCache("index.html", template_index, template_index_size, ctemplate::DO_NOT_STRIP);

    auto router = std::make_unique<router_t>();

    router->http_get("/post/:pid", [](auto req, auto params){
        try {
            auto name = restinio::cast_to<std::string>(params["pid"]);

            ctemplate::TemplateDictionary dict("");
            dict.SetValue("post", std::string(post_data.at(name)));
            auto &n = next_links.at(name);
            auto &p = prev_links.at(name);
            dict.SetValue("next_link", n);
            if (!n.size())
                dict.ShowSection("hnext");
            dict.SetValue("prev_link", p);
            if (!p.size())
                dict.ShowSection("hprev");
            std::string output;
            ctemplate::ExpandTemplate("index.html", ctemplate::DO_NOT_STRIP, &dict, &output);

            req->create_response()
                .append_header(restinio::http_field::content_type, "text/html")
                .set_body(output)
                .done();
        }
        catch (const std::out_of_range& oor) {
            req->create_response( restinio::status_not_found() ).done();
        }
        catch(std::exception const & ex) {
            std::cout << ex.what() << std::endl;
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

