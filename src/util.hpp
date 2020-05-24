#include <vector>
#include <string>
#include <fstream>
#include <sstream>

#include <glob.h>
#include <cmark.h>

namespace util {
    inline std::vector<std::string> glob(std::string path) {
        std::vector<std::string> results;

        glob_t globbuf;
        glob(path.c_str(), 0, nullptr, &globbuf);

        for(size_t i=0; i<globbuf.gl_pathc; i++)
            results.push_back(globbuf.gl_pathv[i]);

        globfree(&globbuf);

        return results;
    }

    inline std::string slurp(std::string path) {
        std::ifstream input(path);
        std::ostringstream sstr;
        sstr << input.rdbuf();
        return sstr.str();
    }

    inline std::string mdToHtml(std::string markdown) {
        char * htmlRaw = cmark_markdown_to_html(markdown.c_str(), markdown.size(), CMARK_OPT_DEFAULT);
        std::string html(htmlRaw);
        free(htmlRaw);
        return html;
    }
}
