##
FROM gentoo/stage3-amd64:latest AS cpp-src-build-core

RUN emerge-webrsync
RUN emerge app-eselect/eselect-repository dev-vcs/git
RUN mkdir -p /etc/portage/repos.conf
RUN eselect repository add localrepo git https://github.com/retupmoca/gentoo-overlay-local
RUN emaint sync -r localrepo

# common dependencies
RUN emerge dev-util/cmake dev-libs/boost

##
FROM cpp-src-build-core AS cpp-webdev-build

RUN echo 'dev-libs/restinio' >>/etc/portage/package.keywords
RUN emerge dev-libs/restinio dev-cpp/ctemplate app-text/cmark

##
FROM cpp-webdev-build AS site-build

WORKDIR /build
COPY . .
RUN make

FROM scratch AS site-deploy

WORKDIR /

COPY --from=site-build /usr/lib64/libcmark.so.0.29.0 /lib64/
COPY --from=site-build /usr/lib64/libhttp_parser.so.2.9 /lib64/
COPY --from=site-build /usr/lib64/libfmt.so.6 /lib64/
COPY --from=site-build /usr/lib64/libctemplate.so.3 /lib64/
COPY --from=site-build /usr/lib/gcc/x86_64-pc-linux-gnu/9.3.0/libstdc++.so.6 /lib64/
COPY --from=site-build /lib64/libm.so.6 /lib64/
COPY --from=site-build /usr/lib/gcc/x86_64-pc-linux-gnu/9.3.0/libgcc_s.so.1 /lib64/
COPY --from=site-build /lib64/libpthread.so.0 /lib64/
COPY --from=site-build /lib64/libc.so.6 /lib64/
COPY --from=site-build /lib64/ld-linux-x86-64.so.2 /lib64/

COPY --from=site-build /build/bin/site /bin/

VOLUME /post

CMD ["/bin/site"]
