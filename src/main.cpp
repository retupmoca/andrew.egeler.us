#include <string>
#include <string_view>
#include <memory>
#include <map>

#include <simple-web-server/server_http.hpp>
#include <jinja2cpp/template.h>
#include <jinja2cpp/template_env.h>

#include <cmark.h>
#include <glob.h>

using HttpServer = SimpleWeb::Server<SimpleWeb::HTTP>;
using std::shared_ptr;

extern "C" {
    extern char template_layout[];
    extern unsigned template_layout_size;

    extern char template_index[];
    extern unsigned template_index_size;

    extern char static_style[];
    extern unsigned static_style_size;

    extern char static_favicon[];
    extern unsigned static_favicon_size;
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

    // http stuff
    HttpServer server;
    server.config.port = 8080;

    jinja2::TemplateEnv env;

    jinja2::Settings settings;
    settings.autoReload = false;
    env.SetSettings(settings);

    auto fs = std::make_shared<jinja2::MemoryFileSystem>();
    fs->AddFile("layout.html", std::string(template_layout, template_layout_size));
    fs->AddFile("index.html", std::string(template_index, template_index_size));
    env.AddFilesystemHandler(std::string(), fs);

    server.resource["^/post/(.*)$"]["GET"] = [&env](shared_ptr<HttpServer::Response> response, [[maybe_unused]] shared_ptr<HttpServer::Request> request) {
        //response->write(post_data.at("posts/foo.md"));
        try {
            auto tpl = env.LoadTemplate("index.html").value();
            auto name = request->path_match[1].str();
            auto output = tpl.RenderAsString({
                {"post", std::string(post_data.at(name))},
                {"next", next_links.at(name)},
                {"prev", prev_links.at(name)},
            });
            response->write(output.value(), {{"Content-Type", "text/html"}});
        }
        catch (const std::out_of_range& oor) {
            response->write(SimpleWeb::StatusCode::client_error_not_found, "Not Found");
        }
        catch(...) {
            std::cout << "ERR" << std::endl;
            response->write("ERR");
        }
    };

    server.resource["^/favicon.ico$"]["GET"] = [](shared_ptr<HttpServer::Response> response, [[maybe_unused]] shared_ptr<HttpServer::Request> request) {
        response->write(std::string_view(static_favicon, static_favicon_size), {{"Content-Type", "image/x-icon"}});
    };
    server.resource["^/style.css$"]["GET"] = [](shared_ptr<HttpServer::Response> response, [[maybe_unused]] shared_ptr<HttpServer::Request> request) {
        response->write(std::string_view(static_style, static_style_size), {{"Content-Type", "text/css"}});
    };

    server.resource["^/$"]["GET"] = [&home_redirect](shared_ptr<HttpServer::Response> response, [[maybe_unused]] shared_ptr<HttpServer::Request> request) {
        try {
            response->write(SimpleWeb::StatusCode::redirection_found, {{"Location", home_redirect}});
        }
        catch (const std::out_of_range& oor) {
            response->write(SimpleWeb::StatusCode::client_error_not_found, "Not Found");
        }
        catch(...) {
            std::cout << "ERR" << std::endl;
            response->write("ERR");
        }
    };

    std::cout << "Starting server." << std::endl;
    server.start();

    return 0;
}

