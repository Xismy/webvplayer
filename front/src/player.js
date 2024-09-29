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

const ImgButton = ({img, onClick}) => <button onClick={onClick}><img src={img} /></button>;
const HideableImgButton = ({img, onClick, bHidden}) => <button className={bHidden? 'Hidden' : 'Visible'} onClick={onClick}><img src={img} /></button>;

export const Player = ({serie, chapter}) => {
	const [state, setState] = useState({serie: null, file: null});
	const [update, setUpdate] = useState(true);

	if(chapter != null && (serie != state.serie || chapter != state.file))
		setState({...state, serie:serie, file: chapter});

	useEffect(() => {
		if(!update) return;
		fetch('http://webvplayer:8008/player')
		.then(response => response.json())
		.then(json => {
			setState(json);
			setUpdate(false);
		});
	}, [update]);

	return state.file == null? <></> :
	<div>
	    <h1>{state.file}</h1>
	    <div className="Player">
		<ImgButton img={rwIcon} onClick={() => ffrw(-5)} />
		<div>
			<HideableImgButton img={stopIcon} bHidden={state.status != Status.PLAYING} onClick={() => {stopVideo(); setUpdate(true)} } />
			<HideableImgButton img={pauseIcon} bHidden={state.status != Status.PLAYING} onClick={() => {pause(); setUpdate(true)} } />
			<HideableImgButton img={playIcon} bHidden={state.status == Status.PLAYING} onClick={() => {playVideo(state); setUpdate(true)} } />
		</div>
		<ImgButton img={ffIcon} onClick={() => ffrw(5)} />
	    </div>
	</div>
}

