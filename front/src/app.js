import logo from '../icons/logo.svg';
import './app.css';
import {Explorer} from './explorer.js';
import {Player} from './player.js';

import {useState, useEffect} from 'react'

function App() {
	const [chapter, setChapter] = useState({serie: null, file: null});

		return (
		<div>
			<h1>webvplayer</h1>
			<Explorer initialSerie={chapter.serie} setChapter={(serie, chapter) => setChapter({serie: serie, file: chapter})} />
			<Player  serie={chapter.serie} chapter={chapter.file}/>
		</div>
  	);
}

export default App;
