let labelOffsetX = 6;
let labelOffsetY = 3;
let circleRadius = 5;

let scale = d3.scaleOrdinal(d3.schemeCategory10);
let color = (d) => scale(d.group);

let links = [];
let nodes = [];
let nodeSet = new Set();

let simulation = d3.forceSimulation(nodes)
    .force("link", d3.forceLink(links).id(d => d.id).distance(0).strength(0.5))
    .force("charge", d3.forceManyBody().strength(-500))
    .force("x", d3.forceX())
    .force("y", d3.forceY());

let resolveDiff = (newNodes) => {
  return newNodes.filter((node) => {
    if (nodeSet.has(node.id)) return false;

    nodeSet.add(node.id);
    return true;
  });
}

let restart = (isHighlighted) => {
    // Draw new nodes & links
    link = d3.select(".links")
        .selectAll("line")
        .data(links)
        .enter().insert("line")
        .attr("style", (d) => `stroke:${isHighlighted(d) ? "red" : "rgb(153,153,153)"}`)
        .attr("stroke-width", (d) => isHighlighted(d) ? 3 : 1);

    node = d3.select(".nodes")
        .selectAll("g")
        .data(nodes)
        .enter().append("g")
        .on("dblclick", (d) => {
            search(d.id, d.group, false)
        });

    circles = node.insert("circle")
        .attr("r", circleRadius)
        .attr("fill", color)
        .call(drag(simulation));

    node = d3.select(".nodes")
        .selectAll("g");

    node.selectAll("text").remove();
    node.append("title")
        .text((d) => {
            return d.id;
        });

    labels = node.append("text")
        .text((d) => {
            return d.id;
        })
        .attr("x", labelOffsetX)
        .attr("y", labelOffsetY);

    console.log(node);

    // Update simulation parameters
    simulation.nodes(nodes);
    simulation.force("link").links(links);
    simulation.alpha(1).restart();
}

const startLoading = () => {
    document.getElementById("overlay").style.display = "block";
    setTimeout(() => {
        document.getElementById("overlay").style.opacity = 1;
    }, 10);
};

const finishLoading = () => {
    document.getElementById("overlay").style.opacity = 0;
    setTimeout(() => {
        document.getElementById("overlay").style.display = "none";
    }, 500);
};

async function search(article, depth, shouldReset, article2) {
    // Get and set new nodes & links

    startLoading();

    const base = "https://wikiwhere.org/api";
    const childUrl = `${base}/children?source=${article}&group=${depth}`;
    const pathUrl = `${base}/path?source=${article}&target=${article2}`;
    const url = article2 ? pathUrl : childUrl;

    let newNodes, newLinks;

    const response = await fetch(url);
    const data = await response.json();
    console.log(data);
    newNodes = resolveDiff(data.nodes);
    newLinks = data.links;
    console.log(newNodes);
    console.log(newLinks);
//  newNodes = [
//    { id: "Article 1", group: 1 },
//    { id: "Article 2", group: 2 },
//    { id: "Article 3", group: 3 },
//    { id: "Article 4", group: 4 },
//  ];
//  newLinks = [
//    { source: "Article 1", target: "Article 2" },
//    { source: "Article 3", target: "Article 2" },
//    { source: "Article 4", target: "Article 1" },
//  ];

    shouldReset && (nodeSet = new Set());
    nodes = shouldReset ? newNodes : [...nodes, ...newNodes];
    links = shouldReset ? newLinks : [...links, ...newLinks];

    let isHighlighted = (d) => false;

    if (data.hasOwnProperty("path")) {
        const path = new Map();

        data.path.forEach((p, i) => {
            path.set(p, i);
        });

        isHighlighted =
            (d) => d && path.has(d.source) && d.target === data.path[path.get(d.source) + 1];
    }

    restart(isHighlighted);

    finishLoading();
};

let ticked = () => {
    node = d3.select(".nodes")
        .selectAll("g");

    link = d3.select(".links")
        .selectAll("line");

    link
        .attr("x1", d => d.source.x)
        .attr("y1", d => d.source.y)
        .attr("x2", d => d.target.x)
        .attr("y2", d => d.target.y);

    node
        .select("circle")
        .attr("cx", d => d.x)
        .attr("cy", d => d.y);

    node
        .select("text")
        .attr("x", d => d.x + labelOffsetX)
        .attr("y", d => d.y + labelOffsetY);
};

drag = simulation => {

    let dragStarted = (d) => {
        if (!d3.event.active) {
            simulation.alphaTarget(0.3).restart();
        }
        d.fx = d.x;
        d.fy = d.y;
    };

    let dragged = (d) => {
        d.fx = d3.event.x;
        d.fy = d3.event.y;
    };

    let dragEnded = (d) => {
        if (!d3.event.active) {
            simulation.alphaTarget(0);
        }
        d.fx = undefined;
        d.fy = undefined;
    };

    return d3.drag()
        .on("start", dragStarted)
        .on("drag", dragged)
        .on("end", dragEnded);
};
let searchBar = d3.select("#search_bar").node().getBoundingClientRect(),
    view = d3.select("#search_bar").node().getBoundingClientRect()
let width = view.width,
    height = window.innerHeight - searchBar.height - 20;
let svg = d3.select("#wikiwhere_view")
    .append("svg")
    .attr("viewBox",
          [-width / 2, -height*2/3 / 2, width, height*2/3])
    .attr("width", width)
    .attr("height", height)
    .call(d3.zoom().on("zoom", function () {
        svg.attr("transform", d3.event.transform)
    }))
    .on("dblclick.zoom", null)
    .append("g");

let link = svg.append("g")
    .attr("class", "links")
    .selectAll("line")
    .data(links)
    .enter().append("line")
    .attr("stroke", "black")
    .attr("stroke-width", 1);

let node = svg.append("g")
    .attr("class", "nodes")
    .selectAll("g")
    .data(nodes)
    .enter().append("g")
    .on("click", (e) => {
        console.log("click", e)
    });

let circles = node.append("circle")
    .attr("r", circleRadius)
    .attr("fill", color)
    .call(drag(simulation))

let labels = node.append("text")
    .text((d) => {
        return d.id;
    })
    .attr("x", labelOffsetX)
    .attr("y", labelOffsetY);

node.append("title")
    .text((d) => {
        return d.id;
    });
simulation
    .nodes(nodes)
    .on("tick", ticked);

simulation.force("link")
    .links(links);
