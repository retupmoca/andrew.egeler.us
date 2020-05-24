#include <string>
#include <string_view>
#include <memory>
#include <map>

#include <restinio/all.hpp>
#include <ctemplate/template.h>

#include "static.hpp"
#include "blog.hpp"

using router_t = restinio::router::express_router_t<>;
using std::string;

int main() {
    ctemplate::StringToTemplateCache("layout.html", template_layout, template_layout_size, ctemplate::DO_NOT_STRIP);
    ctemplate::StringToTemplateCache("index.html", template_index, template_index_size, ctemplate::DO_NOT_STRIP);

    blog::PostMap posts = blog::makePosts("post/*.md");
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

