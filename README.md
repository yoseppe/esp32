How to build docker image:

docker build .

How to find current docker images:

docker images

How to rename docker image:

docker image tag <docker_image_id> <new_repository_name>:<new_tag> 

How to run container from docker image:

docker run -it --rm --privileged --device=<device_name_of_esp32_when_connected> -v ${PWD}:/workspace --entrypoint bash <image_id> 