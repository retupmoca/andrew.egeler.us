#include <vector>
#include <algorithm>

#include <ctemplate/template.h>
#include <cmark.h>
#include <glob.h>

#include "blog.hpp"
#include "util.hpp"

namespace blog {
    using std::vector;
    using std::string;

    struct PostFile {
        string name;
        string filename;
        string prev = "";
        string next = "";
    };
    auto findPostFiles(string basePath) -> vector<PostFile> {
        vector<string> files = util::glob(basePath);
        vector<PostFile> posts;
        string last;

        for(auto &filename : files) {
            string name = filename.substr(5, filename.size() - 8); // TODO: magic numbers, fragile

            posts.push_back({.name=name, .filename=filename});

            if (last.size()) {
                posts[posts.size() - 1].prev = last;
                posts[posts.size() - 2].next = name;
            }
            last = name;
        }

        return posts;
    }

    // TODO: probably want to make this an object to support runtime-updating
    //       of blog post list
    auto makePosts(string basePath) -> PostMap {
        //TODO: I want c++20 ranges for this

        // glob the files
        auto postFiles = findPostFiles(basePath);

        // load the markdown
        struct PostMd {
            string name;
            string markdown;
            string prev;
            string next;
        };
        vector<PostMd> postMarkdown;
        std::transform(postFiles.begin(), postFiles.end(), std::back_inserter(postMarkdown),
            [](PostFile p) -> PostMd {
                return {.name=p.name, .markdown=util::slurp(p.filename), .prev=p.prev, .next=p.next};
            }
        );

        // TODO: generate RSS data from markdown text

        // dump it into HTML templates
        struct PostRendered {
            string name;
            string html;
        };
        vector<PostRendered> postRendered;
        std::transform(postMarkdown.begin(), postMarkdown.end(), std::back_inserter(postRendered),
            [](PostMd p) -> PostRendered {
                // TODO: I don't really like ctemplate's API here
                ctemplate::TemplateDictionary dict("");
                dict.SetValue("post", util::mdToHtml(p.markdown));
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
