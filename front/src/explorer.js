import {useState, useEffect} from 'react'

const Serie = ({name, setSelected}) => <tr key={'serie_'+name}><td key='link'><li onClick={() => setSelected(name)}>{name}</li></td></tr>;
const Chapter = ({chapter, setSelected}) => <tr key={'chapter_' + chapter}><td key='name'><li onClick={() => setSelected(chapter)}>{chapter}</li></td></tr>;

export const Explorer = ({initialSerie, setChapter}) => {
	const [serie, setSerie] = useState(initialSerie);
	const [list, setList] = useState([]);

	useEffect(() => {
		fetch(serie==null? 'http://webvplayer:8008' : 'http://webvplayer:8008/serie/' + serie)
		.then(response => response.json())
		.then(json => setList(json));
	}, [serie]);

	return <div>
	<h1>Explorer</h1>
	<button onClick={() => setSerie(null)}>Back to series</button>
	{serie == null?
	    <></> 
	    : 
	    <div>
		<h1>{serie}</h1>
	    </div>
	}
	<table>
	    <tbody>
	    {list.map((name) => serie == null?
			<Serie name={name} setSelected={setSerie} /> :
		    <Chapter chapter={name} setSelected={(chapter) => setChapter(serie, chapter)} />)
		}
	    </tbody>
	</table>
    </div>
};

