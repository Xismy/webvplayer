import {useState} from 'react'

function save(configObj) {
    let configElem = document.createElement('a');
    let file = new Blob([JSON.stringify(configObj)], {type: 'application/json'});
    configElem.href = URL.createObjectURL(file);
    configElem.download = 'config.json';
    document.body.appendChild(configElem);
    configElem.click();
    document.body.removeChild(configElem);
}

function load(file, setContent) {
    if (file) {
	const reader = new FileReader();
	reader.onload = (e) => {
	    const json = JSON.parse(e.target.result);
	    setContent(json);
	};
	reader.readAsText(file);
    }
}

const Serie = ({serie, setSerie}) => {
    return <tr>
	<td><label>Name: </label><input value={serie.name} onChange={e => setSerie({...serie, name: e.target.value})}/></td>
	<td><label>Path: </label><input value={serie.path} onChange={e => setSerie({...serie, path: e.target.value})}/></td>
	</tr>
}

export const ConfigForm = ({series0}) => {
    const defaultConfig = {
	['front host']: 'webvplayer',
	['front port']: '3000',
	['api host']: 'webvplayer',
	['api port']: '8008',
	['video player']: 'mpv',
	['resources']: series0
    };

    const [config, setConfig] = useState(defaultConfig);

    const setSerie = (newSerie, idx) => {
	let newSeries= [...config.series];
	newSeries[idx] = newSerie;
	setConfig({...config, series: newSeries});
    };

    const addSerie = () => setConfig({...config, series: [...config.series, {name:'', path:''}]});

    return <>
	<table>
	<tr><td><label>Host: </label></td><td><input value={config['front host']} onChange={e => setConfig({...config, ['front host']: e.target.value})}/></td></tr>
	<tr><td><label>Port: </label></td><td><input value={config['front port']} onChange={e => setConfig({...config, ['front port']: e.target.value})}/></td></tr>
	<tr><td><label>API host: </label></td><td><input value={config['api host']} onChange={e => setConfig({...config, ['api_host']: e.target.value})}/></td></tr>
	<tr><td><label>API port: </label></td><td><input value={config['api port']} onChange={e => setConfig({...config, ['api_port']: e.target.value})}/></td></tr>
	<tr>
	<td><label>Video  player: </label></td>
	<td><select onChange={e => setConfig({...config, ['video player']: e.target.value})}>
	<option>mpv</option>
	<option>vlc</option>
	</select></td>
	</tr>
	<tr><td><label>Series</label></td></tr>
	{config.resources.map((serie, idx) => <Serie serie={serie} setSerie={(newSerie) => setSerie(newSerie, idx)} />)}
	</table>
	<button onClick={addSerie}>Add</button>
	<button onClick={() => save(config)}>Save</button>
	<input type='file' accept='.json' onChange={(e) => load(e.target.files[0], setConfig)}/>
	</>
}
