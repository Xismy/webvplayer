import { createSignal, createEffect } from 'solid-js';
import { PlaySvg } from './icons.js'

function load(uri, bPlay) {
	fetch('http://webvplayer:8008/player', {
		method: 'POST',
		contentType: 'application/json',
		body: JSON.stringify({
			action: bPlay ? 'play' : 'load', 
			uri: uri
		})
	});
}

const Resource = ({resource, uri, setUri}) => {
	const subdir = uri() === ''? resource.name : uri() + '/' + resource.name;
	return (
		<div class='resource'>
			<label>{resource.name}</label>
			<Show
				when={resource.mime !== 'application/directory'}
				fallback=<button onClick={() => setUri(subdir)}>Go</button>
			>
				<button onClick={() => load(subdir, false)}>Load</button>
				<button onClick={() => load(subdir, true)}><PlaySvg /></button>
			</Show>
		</div>
	);
}

const Uri = ({uri}) => {
	return <For each={uri().split('/')}>{subdir => <><label>{subdir}</label><label>&gt</label></>}</For>;
}

const Explorer = () => {
	const [uri, setUri] = createSignal('');
	const [resources, setResources] = createSignal([]);

	createEffect(() => {
		fetch('http://webvplayer:8008/list/' + uri())
		.then(response => response.json())
		.then(data => setResources(data));
	});

	return (
	<div class='explorer'>
		<h1>
			<Show
				when={uri() !== ''}
				fallback={<></>}
			>
				<button onClick={() => setUri(uri().split('/').slice(0, -1).join('/'))}>Back</button>
			</Show>
			<Uri uri={uri}/>
		</h1>
		<div class='list'>
			<For each={resources()}>{resource => <Resource resource={resource} uri={uri} setUri={setUri} />}</For>
		</div>
	</div>);
}

export {Explorer};
