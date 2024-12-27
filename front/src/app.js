import logo from '../icons/logo.svg';
import './app.css';
import {Explorer} from './explorer.js';
import {Player} from './player.js';

import {useState, useEffect} from 'react'

const HoldButton = ({id, text, onHold, timeoutMs, updateResolutionMs}) => {
	const [state, setState] = useState({timer: null, interval:null, progress: 0});
	const startTimer = () => {
		let timer = setTimeout(onHold, timeoutMs);
		let interval = setInterval(() => {
			setState((oldState) => {
				thisElement = document.getElementById(id);
				let progress = oldState.progress + updateResolutionMs;
				thisElement.style.width = (50 + progress * 50 / timeoutMs) + '%';
				return {timer: oldState.timer, interval: oldState.interval, progress: progress};
			});
		}, updateResolutionMs);
		setState({timer: timer, interval: interval, progress: 0});
	};
	const stopTimer = () => {
		clearTimeout(state.timer);
		clearInterval(state.interval);
		setState({timer: null, interval: null, progress: 0});
	};
	return <button id={id} onMouseDown={startTimer} onMouseUp={stopTimer} onMouseLeave={stopTimer} onTouchStart={startTimer} onTouchEnd={stopTimer} onTouchCancel={stopTimer}>{text}</button>
};

function App() {
	const [chapter, setChapter] = useState({serie: null, file: null});

		return (
		<div>
			<aside>
				<HoldButton id='powerButton' text='Poweroff' onHold={() => fetch('http://webvplayer:8008/poweroff', {method: 'POST'})} timeoutMs={3000} updateResolutionMs={100} />
			</aside>
			<h1>webvplayer</h1>
			<Explorer initialSerie={chapter.serie} setChapter={(serie, chapter) => setChapter({serie: serie, file: chapter})} />
			<Player  serie={chapter.serie} chapter={chapter.file}/>
		</div>
  	);
}

export default App;
