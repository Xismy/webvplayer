import {createSignal, createEffect} from 'solid-js';
import {PlaySvg, PauseSvg, StopSvg} from './icons.js'

const HostUrl = 'webvplayer:8008';
const PlayerEndpoint = 'http://' + HostUrl + '/player';
const PlayerUpdateEndpoint = 'ws://' + HostUrl + '/player_update';


function processUpdate(update, state, setState) {
	let newState = {...state()};

	switch (update.action) {
		case 'play':
			newState['state'] = 'playing';
		case 'load':
			newState['uri'] = update['uri'];
			newState['audio-tracks'] = update['audio-tracks'];
			newState['subs-tracks'] = update['subs-tracks'];
			break;
		case 'resume':
		case 'pause':
			newState['state'] = update.action === 'pause' ? 'paused' : 'playing';
		case 'goto':
			newState['time-pos'] = update['time-pos'];
			break;
		case 'stop':
			setState({uri: null});
			return;
		default:
			break;
	}

	setState(newState);
}

function sendAction(action, params={}) {
	fetch(PlayerEndpoint, {
		method: 'POST',
		body: JSON.stringify({action: action, ...params})
	});
}

const Player = () => {
	const [state, setState] = createSignal({uri: null});
	
	createEffect(() => {
		fetch(PlayerEndpoint)
		.then(response => response.json())
		.then(data => setState(data));
	});

	createEffect(() => {
		new WebSocket(PlayerUpdateEndpoint).onmessage = event => {
			processUpdate(JSON.parse(event.data), state, setState);
		}
	});

	return (
		<Show
			when={state().uri !== null}
			fallback={<></>}
		>
			<h1>{state().uri}</h1>
			<div class='player'>
				<button onClick={() => sendAction('goto', {value: -10})}>RW 10s</button>
				<button onClick={() => sendAction('goto', {value: -5})}>RW 5s</button>
				<div>
					<Show 
						when={state().state === 'playing'}
						fallback={<button onClick={() => sendAction('resume')}><PlaySvg /></button>}
					>	
						<button onClick={() => sendAction('pause')}><PauseSvg /></button>
					</Show>
					<button onClick={() => sendAction('stop')}><StopSvg /></button>
				</div>
				<button onClick={() => sendAction('goto', {value: 5})}>FF 5s</button>
				<button onClick={() => sendAction('goto', {value: 10})}>FF 10s</button>
			</div>
			<div>
				<label>Volume</label>
				<input type='range'/>
			</div>
			<div>
				<label>Audio</label>
				<label>Subtitles</label>
			</div>
		</Show>
	);
}

export {Player};
