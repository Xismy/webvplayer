const fs = require('fs').promises;
const path = require('path');
const babel = require('@babel/core');

async function parse_icon(icon) {
	try {
		const content = await fs.readFile('src/icons/' + icon + '.svg', 'utf8');
		const prefix = 'export const ' + icon[0].toUpperCase() + icon.slice(1) + 'Svg = () => { return (';
		const suffix = ')}';
		return prefix + content.match(/<svg.*<\/svg>/s)[0] + suffix;
	} catch (err) {
		console.error('Error parsing ' + icon, err.message);
	}
}

async function gen_icons() {
	const icons = await Promise.all(['play', 'pause', 'stop', 'ff', 'next', 'home', 'watched', 'nonWatched'].map(parse_icon));
	babel.transform(icons.join('\n\n'), {
			presets: ['babel-preset-solid'],
		}, (err, result) => {
			if (err) {
				console.error('Error transpiling icons:', err);
				return;
			}
			fs.writeFile('dist/icons.js', result.code, 'utf8')
				.then(() => console.log('✅ Icons transpiled to icons.js'))
				.catch(err => console.error('Error writing icons.js:', err));
		});
}

gen_icons();
