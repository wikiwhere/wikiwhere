let width = 960,
    height = 500;

let scale = d3.scaleOrdinal(d3.schemeCategory10);

let labelOffsetX = 6,
    labelOffsetY = 3,
    circleRadius = 5;

let data = {
    nodes: [
        {id: "Article 1", group: 1},
        {id: "Article 2", group: 2},
    ],
    links: [
        {source: "Article 1", target: "Article 2"},
    ]
};

let svg = d3.select("svg")
    .attr("viewBox", [-width / 2, -height / 2, width, height]);

let nodes = [];
let links = [];

let simulation = d3.forceSimulation(nodes)
    .force("link", d3.forceLink(links).id(d => d.id).distance(0).strength(1))
    .force("charge", d3.forceManyBody().strength(-50))
    .force("x", d3.forceX())
    .force("y", d3.forceY());

let link = svg.append("g")
        .attr("class","links")
        .selectAll("line"),
    node = svg.append("g")
        .attr("class", "nodes")
        .selectAll("g");

let search = () => {
    let newNodes = [
        {
            id: "Article 3",
            group: 3
        },
        {
            id: "Article 4",
            group: 4
        }];
    let newLinks = [
        {source: "Article 4", target: "Article 3"},
    ];

    nodes = [...nodes, ...newNodes.map(d => Object.create(d))];
    links = [...links, ...newLinks.map(d => Object.create(d))];

    restart();
};

let ticked = () => {
    link = d3.select(".links")
        .selectAll("line");

    node = d3.select(".nodes")
        .selectAll("g");

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

let drag = simulation => {

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
        d.fx = null;
        d.fy = null;
    };

    return d3.drag()
        .on("start", dragStarted)
        .on("drag", dragged)
        .on("end", dragEnded);
};

let color = (d) => scale(d.group);

let restart = () => {
    link = link.data(links);

    link.enter().append("line")
        .attr("stroke", "black")
        .attr("stroke-width", 1);

    node = node.data(nodes)
        .enter().append("g");

    let circles = node.append("circle")
        .attr("r", circleRadius)
        .attr("fill", color)
        .call(drag(simulation));

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
};

restart();