import { createSignal, createEffect, createMemo } from 'solid-js';
import { HomeSvg, PlaySvg } from './icons.js'

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
	const callback = resource.mime === 'application/directory' ? () => setUri(subdir) : undefined;
	return (
		<div onClick={callback} class='resource'>
			<label>{resource.name}</label>
			<Show
				when={resource.mime !== 'application/directory'}
			>
				<button onClick={() => load(subdir, false)}><PlaySvg /></button>
			</Show>
		</div>
	);
}

const PathBar = ({uri, setUri}) => {
	const uriParts = createMemo(() => uri().split('/').filter(s => s !== ''));
	const goToDir = (n) => {setUri(uriParts().slice(0, n).join('/'))};
	return <div class='path-bar'>
		<label><button onClick={() => goToDir(0)}><HomeSvg /></button></label>
		<For each={uriParts()}>{(subdir, i) => <><label onClick={() => goToDir(i()+1)}>{subdir}</label></>}</For></div>;
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
		<Show
			when={uri() !== ''}
			fallback={<></>}
		>
		</Show>
		<PathBar uri={uri} setUri={setUri} />
		<div class='list'>
			<For each={resources()}>{resource => <Resource resource={resource} uri={uri} setUri={setUri} />}</For>
		</div>
	</div>);
}

export {Explorer};
