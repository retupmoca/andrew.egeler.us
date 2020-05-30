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
    auto makePosts(string basePath) -> PostData {
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

        // generate RSS data from markdown text
        // note: this is a for loop because by spec we can't rely on
        //       std::transform going in-order
        std::string rss;
        {
            ctemplate::TemplateDictionary dict("");
            for(auto &post : postMarkdown) {
                // TODO: ew, manual parsing. This should either be hidden, or
                //       should use cmark to do the parsing
                auto postDict = dict.AddSectionDictionary("POST");
                postDict->SetValue("name", post.name);

                // assume that line 1 is a markdown heading
                unsigned titleEnd = 0;
                char const * p = post.markdown.c_str();
                for(; p[titleEnd] && p[titleEnd] != '\n'; titleEnd++)
                    ;
                if(titleEnd < 2 || !p[titleEnd] || p[titleEnd+1] != '\n')
                    continue;
                std::string title = post.markdown.substr(2, titleEnd-2);
                postDict->SetValue("title", title);

                // assume that the paragraph after the title is a TL;DR:
                unsigned tldrEnd = 0;
                p = p + titleEnd + 2;
                for(; p[tldrEnd] && !(p[tldrEnd] == '\n' && p[tldrEnd + 1] == '\n'); tldrEnd++)
                    ;
                std::string description;
                if(p[tldrEnd])
                    description = post.markdown.substr(titleEnd+2, tldrEnd);
                postDict->SetValue("description", description);
            }
            ctemplate::ExpandTemplate("rss.xml", ctemplate::DO_NOT_STRIP, &dict, &rss);
        }

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
        return {.posts=posts, .newestPostName=postRendered.back().name, .rss=rss};
    }
}
