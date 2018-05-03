.PHONY: help deploy
.DEFAULT_GOAL: help

help:
	@echo "make deploy"

deploy:
	rsync -rv htdocs/ /srv/andrew.egeler.us/
