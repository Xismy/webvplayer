import React from 'react';
import ReactDOM from 'react-dom/client';
import {ConfigForm} from '../src/config';

const app = ReactDOM.createRoot(document.getElementById('app'));
app.render(
  <React.StrictMode>
    <ConfigForm series0={[]} />
  </React.StrictMode>
);
