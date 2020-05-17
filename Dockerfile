##
FROM gentoo/stage3-amd64:latest AS cpp-src-build-core

RUN emerge-webrsync
RUN emerge app-eselect/eselect-repository dev-vcs/git
RUN mkdir -p /etc/portage/repos.conf
RUN eselect repository add localrepo git https://github.com/retupmoca/gentoo-overlay-local
RUN emaint sync -r localrepo

# common dependencies
RUN echo 'dev-util/ldd-dep-cp' >>/etc/portage/package.keywords
RUN emerge dev-util/cmake dev-libs/boost dev-util/ldd-dep-cp

##
FROM cpp-src-build-core AS cpp-webdev-build

RUN echo 'dev-libs/restinio' >>/etc/portage/package.keywords
RUN emerge dev-libs/restinio dev-cpp/ctemplate app-text/cmark

##
FROM cpp-webdev-build AS site-build

WORKDIR /build
COPY . .
RUN make
RUN mkdir -p /dist/lib64
RUN mkdir -p /dist/bin
RUN cp bin/site /dist/bin
RUN ldd-dep-cp bin/site /dist/lib64

FROM scratch AS site-deploy

WORKDIR /

COPY --from=site-build /dist/ /

VOLUME /post

CMD ["/bin/site"]
