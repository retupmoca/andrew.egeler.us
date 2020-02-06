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

RUN echo 'dev-libs/simple-web-server' >>/etc/portage/package.keywords
RUN echo 'dev-libs/jinja2cpp' >>/etc/portage/package.keywords
RUN emerge dev-libs/simple-web-server dev-libs/jinja2cpp app-text/cmark

##
FROM cpp-webdev-build AS site-build

WORKDIR /build
COPY . .
RUN make

FROM scratch AS site-deploy

WORKDIR /

COPY --from=site-build /lib64/libpthread.so.0 /lib64/
COPY --from=site-build /usr/lib64/libboost_filesystem.so.1.72.0 /lib64/
COPY --from=site-build /usr/lib64/libcmark.so.0.29.0 /lib64/
COPY --from=site-build /usr/lib/gcc/x86_64-pc-linux-gnu/9.2.0/libstdc++.so.6 /lib64/
COPY --from=site-build /lib64/libm.so.6 /lib64/
COPY --from=site-build /usr/lib/gcc/x86_64-pc-linux-gnu/9.2.0/libgcc_s.so.1 /lib64/
COPY --from=site-build /lib64/libc.so.6 /lib64/
COPY --from=site-build /lib64/ld-linux-x86-64.so.2 /lib64/
COPY --from=site-build /lib64/librt.so.1 /lib64/

COPY --from=site-build /build/bin/site /bin/

VOLUME /post

CMD ["/bin/site"]
