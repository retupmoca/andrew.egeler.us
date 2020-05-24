#include <vector>
#include <algorithm>

#include <ctemplate/template.h>
#include <cmark.h>
#include <glob.h>

#include "blog.hpp"

namespace blog {
    using std::vector;
    using std::string;

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
}
