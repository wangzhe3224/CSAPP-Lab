image = "zwang/csapp"
path="/Users/zhewang/Projects/CSAPP-Lab"  

if ! docker container rm csapp_env; then
    echo "remove old container."
else
    echo "no old container exist. Create a new one"
fi

docker container run -it -v ${path}:/csapp --name=csapp_env ${image} /bin/bash
