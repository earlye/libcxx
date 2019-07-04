#!/usr/bin/env bash

set -e
set -x

if [[ -z "$GITLAB_CONFIG_VOLUME" ]]; then
  echo "Error, GITLAB_CONFIG_VOLUME must be defined"
  exit 1
fi
if [[ ! -d "$GITLAB_SCRIPTS" ]]; then
  echo "Error, GITLAB_SCRIPTS must be defined"
  exit 1
fi

export PATH=$PATH:$GITLAB_SCRIPTS/

sudo service docker start

docker run  --rm  \
   -v $GITLAB_CONFIG_VOLUME:/etc/gitlab-runner \
   -v /var/run/docker.sock:/var/run/docker.sock \
   gitlab/gitlab-runner register \
  --non-interactive \
  --executor "docker" \
  --docker-image ericwf/llvm-gitlab-runner-worker:latest \
  --url "https://gitlab.com/" \
  --registration-token "$TOKEN" \
  --description "llvm-docker-runner" \
  --tag-list "docker,google-cloud" \
  --run-untagged="true" \
  --locked="false" \
  --access-level="not_protected"