import playIcon from '../icons/play.svg';
import pauseIcon from '../icons/pause.svg';
import stopIcon from '../icons/stop.svg';
import ffIcon from '../icons/ff.svg';
import rwIcon from '../icons/rw.svg';
import {useState, useEffect} from 'react'

const Status = {
	IDLE: 0,
	PLAYING:1,
	PAUSED: 2
}

function playVideo(state) {
	if(state.status != Status.PAUSED) {
		const json = {action: 'play', serie: state.serie, file: state.file};
		fetch('http://webvplayer:8008/player', {method: 'POST', body: JSON.stringify(json)});
	}
	else {
		const json = {action: 'resume'}
		fetch('http://webvplayer:8008/player', {method: 'POST', body: JSON.stringify(json)});
	}
}

function pause() {
	const json = {action: 'pause'}
	fetch('http://webvplayer:8008/player', {method: 'POST', body: JSON.stringify(json)});
}

function stopVideo() {	
	const json = {action: 'stop'}
	fetch('http://webvplayer:8008/player', {method: 'POST', body: JSON.stringify(json)});
}

function ffrw(delta) {
	const json = {action: 'goto', value: delta};
	fetch('http://webvplayer:8008/player', {method: 'POST', body: JSON.stringify(json)});
}

function processSocketEvent(eventData, setState) {
	setState((oldState) => {
		state = {...oldState}
		switch(eventData.action) {
			case 'play':
				state.status = Status.PLAYING;
				state.serie = eventData.serie;
				state.file = eventData.file;
				break;
			case 'stop':
				state.status = Status.IDLE;
				break;
			case 'pause':
				state.status = Status.PAUSED;
				break;
			case 'resume':
				state.status = Status.PLAYING;
				break;
		}

		console.log(JSON.stringify(state));

		return state;
	});
}

function selectAudioTrack(track) {
	const json = {action: 'select-audio-track', track: track==''? null : track};
	fetch('http://webvplayer:8008/player', {method: 'POST', body: JSON.stringify(json)});
}

function selectSubtitleTrack(track) {
	const json = {action: 'select-subs-track', track: track==''? null : track};
	fetch('http://webvplayer:8008/player', {method: 'POST', body: JSON.stringify(json)});
}

const ImgButton = ({img, onClick}) => <button onClick={onClick}><img src={img} /></button>;
const HideableImgButton = ({img, onClick, bHidden}) => <button className={bHidden? 'Hidden' : 'Visible'} onClick={onClick}><img src={img} /></button>;
const TrackSelector = ({title, tracks, onChange}) => <div>
	<label>{title}</label>
	<select onChange={onChange}>
		<option value=''>None</option>
		{tracks.map((track, index) => <option key={index} value={track.id}>{track.title}</option>)}
	</select>
</div>;

export const Player = ({serie, chapter}) => {
	const [state, setState] = useState({serie: null, file: null});
	
	if(chapter != null && (serie != state.serie || chapter != state.file))
		setState({...state, serie:serie, file: chapter});

	useEffect(() => {
		new WebSocket('ws://webvplayer:8008/player_update').onmessage = (event) => { processSocketEvent(JSON.parse(event.data), setState); }

		fetch('http://webvplayer:8008/player')
		.then(response => response.json())
		.then(json => {
			setState(json);
		});
	}, []);

	return state.file == null? <></> :
	<div>
	    <h1>{state.file}</h1>
	    <div className="Player">
		<ImgButton img={rwIcon} onClick={() => ffrw(-5)} />
		<div>
			<HideableImgButton img={stopIcon} bHidden={state.status != Status.PLAYING} onClick={() => {stopVideo();} } />
			<HideableImgButton img={pauseIcon} bHidden={state.status != Status.PLAYING} onClick={() => {pause();} } />
			<HideableImgButton img={playIcon} bHidden={state.status == Status.PLAYING} onClick={() => {playVideo(state);} } />
		</div>
		<ImgButton img={ffIcon} onClick={() => ffrw(5)} />
	    </div>
		{!state.hasOwnProperty('audio-tracks')? <></> : 
			<TrackSelector 
				title='Audio' 
				tracks={state['audio-tracks']}
				onChange={(event) => {selectAudioTrack(event.target.value)}}
			/>
		}
		{!state.hasOwnProperty('subs-tracks')? <></> : 
			<TrackSelector 
				title='Subtitles' 
				tracks={state['subs-tracks']}
				onChange={(event) => {selectSubtitleTrack(event.target.value)}}
			/>
		}
	</div>
}

