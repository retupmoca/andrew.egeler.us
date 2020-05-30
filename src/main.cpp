#include <string>
#include <string_view>
#include <memory>
#include <map>
#include <thread>

#include <restinio/all.hpp>
#include <ctemplate/template.h>
#include <sys/inotify.h>

#include "static.hpp"
#include "blog.hpp"

using router_t = restinio::router::express_router_t<>;
using std::string;

// call cb once for every event
void watchFolderAsync(std::function<void(void)> cb) {
    auto thread = std::make_unique<std::thread>([cb]() {
        int fd = inotify_init();
        if(fd < 0) return;
        int wd = inotify_add_watch(fd, "post",
            IN_CREATE | IN_MODIFY | IN_MOVED_TO | IN_CLOSE_WRITE
        );
        if(wd < 0) return;
        size_t bsize = 512 + sizeof(struct inotify_event);
        while(1) {
            char buf[bsize];
            int len = read(fd, buf, bsize);
            if (len < 0)
                break;
            cb();
        }
    });
    thread->detach();
}

int main() {
    ctemplate::StringToTemplateCache("rss.xml", template_rss, template_rss_size, ctemplate::DO_NOT_STRIP);
    ctemplate::StringToTemplateCache("index.html", template_index, template_index_size, ctemplate::DO_NOT_STRIP);

    bool needUpdate = true;
    watchFolderAsync([&needUpdate](){
        needUpdate = true;
    });
    blog::PostData rendered;
    auto maybeReloadPosts = [&needUpdate, &rendered]() {
        if(needUpdate) {
            std::cout << "Reloading\n";
            needUpdate = false;
            rendered = blog::makePosts("post/*.md");
        }
    };

    auto router = std::make_unique<router_t>();

    router->http_get("/post/:pid", [&rendered, &maybeReloadPosts](auto req, auto params){
        try {
            maybeReloadPosts();
            auto name = restinio::cast_to<std::string>(params["pid"]);
            auto html = rendered.posts.at(name);

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

    router->http_get("/", [&rendered, &maybeReloadPosts](auto req, [[maybe_unused]] auto params){
        try {
            maybeReloadPosts();
            req->create_response(restinio::status_found())
                .append_header(restinio::http_field::location, "/post/" + rendered.newestPostName)
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

