.PHONY: deploy
deploy:
	rsync -rv htdocs/ /srv/andrew.egeler.us/
