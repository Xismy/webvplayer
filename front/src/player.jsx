import {createSignal, createEffect} from 'solid-js';
import {PlaySvg, PauseSvg, StopSvg, FfSvg, NextSvg} from './icons.jsx'



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
	fetch('/api/player', {
		method: 'POST',
		body: JSON.stringify({action: action, ...params})
	});
}

const TimeSkipButton = ({time}) => {
	return (
		<button onClick={() => sendAction('goto', {value: time})}><div style={time < 0? 'scale: -1' : ''}><FfSvg /></div><label>{time}</label></button>
	);
}

const Player = () => {
	const [state, setState] = createSignal({uri: null});
	
	createEffect(() => {
		fetch('/api/player')
		.then(response => response.json())
		.then(data => setState(data));
	});

	createEffect(() => {
		new WebSocket(`wss://${location.host}/api/player_update`)
			.onmessage = event => {
			processUpdate(JSON.parse(event.data), state, setState);
		}
	});

	return (
		<div class='player'>
			<Show
				when={state().uri !== null}
				fallback={<></>}
			>
				<h1>{state().uri}</h1>
				<div class='button-box'>
					<div class='button-col'>
						<TimeSkipButton time={-60} />
						<TimeSkipButton time={-15} />
						<TimeSkipButton time={-3} />
					</div>
					<div class='button-col'>
						<Show 
							when={state().state === 'playing'}
							fallback={<button onClick={() => sendAction('resume')}><PlaySvg /></button>}
						>	
							<button onClick={() => sendAction('pause')}><PauseSvg /></button>
						</Show>
						<button onClick={() => sendAction('stop')}><StopSvg /></button>
					</div>
					<div class='button-col'>
						<TimeSkipButton time={60} />
						<TimeSkipButton time={15} />
						<TimeSkipButton time={3} />
					</div>
				</div>
				<div>
					<label>Volume</label>
					<input type='range' min='0' max='130' value={state().volume} onchange={e => sendAction('set-volume', {volume: e.target.value})} />
					<label>{state().volume}</label>
				</div>
				<div>
					<label>Audio</label>
					<select onchange={e => sendAction('select-audio-track', {track: e.target.value})}>
						<For each={state()['audio-tracks']}>{track => <option value={track.id}>{track.title}</option>}</For>
					</select>
				</div>
				<div>
					<label>Subtitles</label>
					<select onchange={e => sendAction('select-subs-track', {track: e.target.value})}>
						<option value='none'>None</option>
						<For each={state()['subs-tracks']}>{track => <option value={track.id}>{track.title}</option>}</For>
					</select>
				</div>
				<div>
					<label>Time position: {state()['time-pos']}</label>
				</div>
			</Show>
		</div>
	);
}

export {Player};
