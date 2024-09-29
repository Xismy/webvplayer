## Build steps
### Rest API
````sh
cmake -B rest/build -G Ninja rest
ninja -C rest/build -j 4
````
### Front end
````sh
npm install --prefix front
````
## Run steps
You need a configuration file to start the rest API. A sample is provided in `rest/webvplayer-default.json`
````sh
rest/build/webvplayer --config <path/to/your/config/file>
npm start --prefix front
````
