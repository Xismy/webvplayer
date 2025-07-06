import {render} from 'solid-js/web';
import {Explorer} from './explorer.js';
import {Player} from './player.js';

render(() => <><Explorer/><Player/></>, document.getElementById('app'))
