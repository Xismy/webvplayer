import React from 'react';
import ReactDOM from 'react-dom/client';
import './index.css';
import App from '../src/app';

const app = ReactDOM.createRoot(document.getElementById('app'));
app.render(
  <React.StrictMode>
    <App />
  </React.StrictMode>
);
