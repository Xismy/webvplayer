## Build steps
### Rest API
````sh
cmake -B rest/build -G Ninja rest
ninja -C rest/build -j 4
````
### Front end
````sh
npm run build --prefix front
````
## Run steps
### Rest API
You need a configuration file to start the rest API. A sample is provided in `rest/webvplayer-default.json`
````sh
rest/build/webvplayer --config <path/to/your/config/file>
````
### Front end
You need to setup a web server (e.g. nginx or apache) and provide the dist directory (front/dist) as the http root directory (index.html as deafult resource).
