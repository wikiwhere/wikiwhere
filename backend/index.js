const express = require('express');
const child_process = require('child_process');
const path = require('path');
const app = express();

const pageDbPath = process.env.PAGE_PATH || '../../testDb/enwikibooks-20191001-page.db';
const pagelinksDbPath = process.env.PAGELINKS_PATH || '../../testDb/enwikibooks-20191001-pagelinks.db';

const run = (cmd, args, options) => {
  let completed = false;
  return new Promise((resolve, reject) => {
    const child = child_process.spawn(cmd, args, options);
    let data = "";

    child.stdout.on('data', (buffer) => { data += buffer.toString() });
    child.stdout.on('end', () => {
      if (!completed) {
        resolve(data)
      };
      completed = true;
    });
    child.stdout.on('close', (errCode) => {
      if (errCode !== 0 && !completed) {
        reject(data)
      }
      completed = true;
    });
  });
}

const runSearch = (args) => {
  return run(
    './searchTargetWiki',
    [pageDbPath, pagelinksDbPath, ...args],
    { cwd: path.join(__dirname, '..', 'searchAlgo', 'build') }
  );
}

app.use(express.static(path.join(__dirname, '..', 'frontend')));

app.get('/api/children', async (req, res) => {
  const { source, group } = req.query;

  const result = await runSearch([ 'children', source, group ]);
  const json = JSON.parse(result);

  res.status(json.status).json(json.data);
});

app.get('/api/path', async (req, res) => {
  const { source, target } = req.query;

  const result = await runSearch([ 'path', source, target ]);
  const json = JSON.parse(result);

  res.status(json.status).json(json.data);
});

const server = app.listen(process.env.PORT || 5000, () => {
  const host = server.address().address;
  const port = server.address().port;
  
  console.log(`Server listening at http://${host}:${port}`);
});