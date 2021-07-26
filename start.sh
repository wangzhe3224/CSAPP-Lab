image = "zwang/csapp"
path="/Users/zhewang/Projects"  

if ! docker container rm csapp_env; then
    echo "remove old container."
else
    echo "no old container exist. Create a new one"
fi

docker container run -it -v ${path}:/projects -p 8080:8080 --name=csapp_env --net host wangzhe3224/csapp /bin/bash
