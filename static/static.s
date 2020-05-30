    .section .rodata

    .global template_rss
    .type   template_rss, @object
    .align  4
template_rss:
    .incbin "static/rss.xml"
template_rss_end:
    .global template_rss_size
    .type   tempalte_rss_size, @object
    .align  4
template_rss_size:
    .int    template_rss_end - template_rss

    .global template_index
    .type   template_index, @object
    .align  4
template_index:
    .incbin "static/index.html"
template_index_end:
    .global template_index_size
    .type   template_index_size, @object
    .align  4
template_index_size:
    .int    template_index_end - template_index

    .global static_style
    .type   static_style, @object
    .align  4
static_style:
    .incbin "static/style.css"
static_style_end:
    .global static_style_size
    .type   static_style_size, @object
    .align  4
static_style_size:
    .int    static_style_end - static_style

    .global static_favicon
    .type   static_favicon, @object
    .align  4
static_favicon:
    .incbin "static/favicon.ico"
static_favicon_end:
    .global static_favicon_size
    .type   static_favicon_size, @object
    .align  4
static_favicon_size:
    .int    static_favicon_end - static_favicon
