piechartsvg
  .selectAll("mySlices")
  .data(data_ready)
  .enter()
  .append("text")
  .text(function (d) {
    if (d.endAngle - d.startAngle < 0.1) return "";
    let value = d.data.value.toLocaleString();
    return `${value} ${d.data.animal}`;
  })
  .attr("transform", function (d) {
    // return "translate(" + arc_generator.centroid(d) + ")";
    // if text doesn't fit, don't show it
    // rotate the text 45 degrees
    return "translate(" + arc_generator.centroid(d) + ")";
  })
  .style("text-anchor", "middle")
  .style("font-size", 13);
