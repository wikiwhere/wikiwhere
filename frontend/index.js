let width = 960,
    height = 600;

let scale = d3.scaleOrdinal(d3.schemeCategory10);
let color = (d) => scale(d.group);

let data = {
  "nodes": [
    {"id": "Article 1",
     "group": 1 },
    {"id": "Article 2",
     "group": 2 },
  ],
  "links": [
    {"source": "Article 1", "target": "Article 2"},
  ]
};

let links = data.links.map(d => Object.create(d));
let nodes = data.nodes.map(d => Object.create(d));

let simulation = d3.forceSimulation(nodes)
  .force("link", d3.forceLink(links).id(d => d.id))
  .force("charge", d3.forceManyBody())
  .force("center", d3.forceCenter(width / 2, height / 2));

let ticked = () => {
	link
    .attr("x1", d => d.source.x)
    .attr("y1", d => d.source.y)
    .attr("x2", d => d.target.x)
    .attr("y2", d => d.target.y);

  // node
  //   .attr("cx", d => d.x)
  //   .attr("cy", d => d.y);
	node
    .attr("transform", function(d) {
      return "translate(" + d.x + "," + d.y + ")";
    });
};

drag = simulation => {
  
  let dragstarted = (d) => {
      if (!d3.event.active) simulation.alphaTarget(0.3).restart();
      d.fx = d.x;
      d.fy = d.y;
    }
  
  let dragged = (d) => {
      d.fx = d3.event.x;
      d.fy = d3.event.y;
    }
  
  let dragended = (d) => {
      if (!d3.event.active) simulation.alphaTarget(0);
      d.fx = null;
      d.fy = null;
    }
  
  return d3.drag()
      .on("start", dragstarted)
      .on("drag", dragged)
      .on("end", dragended);
}

let svg = d3.select("svg")

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
  
let circles = node.append("circle")
  .attr("r", 5)
  .attr("fill", color)
  .call(drag(simulation));

let labels = node.append("text")
    .text((d) => {
      return d.id;
    })
    .attr('x', 6)
    .attr('y', 3);

node.append("title")
    .text((d) => { return d.id; });

simulation
  .nodes(nodes)
  .on("tick", ticked);

simulation.force("link")
  .links(links);
