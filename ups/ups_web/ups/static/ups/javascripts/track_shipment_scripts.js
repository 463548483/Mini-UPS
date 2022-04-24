function initCanvas() {
    var canvas = document.getElementById("my-canvas");
    var ctx = canvas.getContext("2d");
    var moveUnit = 20

    // trucks status from database
    let trucks_string = document.getElementById("trucks")
    const trucks = JSON.parse(trucks_string.innerText);

    // truck image
    var test = new Image();
    truckImg = document.getElementById("truck");
    truckImg.onload = function() {
        var map = new Map();
        map.grid_size = 16;

        function animate(){
            ctx.fillStyle = "white";
            ctx.fillRect(0, 0, canvas.width, canvas.height);
            ctx.fillStyle = "black";
            map.render();
        }
        var animateInterval = setInterval(animate, 300);

        // user interactive
        // dragging the map
        var startPosX;
        var startPosY;
        canvas.onmousedown = (e) => {
            startPosX = e.layerX;
            startPosY = e.layerY;
        }
        canvas.onmouseup = (e) => {
            deltaX = e.layerX - startPosX;
            deltaY = e.layerY - startPosY;
            // console.log("(dx, dy) = (", deltaX, ", ", deltaY, ")");

            map.x_axis_distance_grid_lines += Math.floor(deltaY / 5);
            map.y_axis_distance_grid_lines += Math.floor(deltaX / 5);
        }

        // pressing key
        document.addEventListener('keydown', function(event) {
            var key_press = String.fromCharCode(event.keyCode);
            // console.log(key_press);
            if (key_press == "»") {
                console.log("User wants to scale up");
                // can't scale up too much
                if (map.grid_size <= 30) {
                    map.grid_size += 2;
                }
            } else if (key_press == "½") {
                console.log("User wants to scale down");
                // can't scale down too much
                if (map.grid_size >= 10) {
                    map.grid_size -= 2;
                }
            } else if (key_press == "W") {
                console.log("User wants to move up");
                map.x_axis_distance_grid_lines += 10;
            } else if (key_press == "S") {
                console.log("User wants to move down");
                map.x_axis_distance_grid_lines -= 10;
            } else if (key_press == "A") {
                console.log("User wants to move left");
                map.y_axis_distance_grid_lines += 10;
            } else if (key_press == "D") {
                console.log("User wants to move right");
                map.y_axis_distance_grid_lines -= 10;
            }
        });
    }

    function Map() {
        // default value
        // grid size is 10 px
        this.grid_size = 10;
        // location of the x, y main axis
        this.x_axis_distance_grid_lines = 20;
        this.y_axis_distance_grid_lines = 20;
        // axis starting number
        this.x_axis_starting_point = { number: 1, suffix: '' };
        this.y_axis_starting_point = { number: 1, suffix: '' };
        // canvas width and height
        this.canvas_width = canvas.width;
        this.canvas_height = canvas.height;

        this.render = function() {

            var num_lines_x = Math.floor(this.canvas_height/this.grid_size);
            var num_lines_y = Math.floor(this.canvas_width/this.grid_size);

            // Draw grid lines along X-axis
            for(var i=0; i<=num_lines_x; i++) {
                ctx.beginPath();
                ctx.lineWidth = 1;
                
                // If line represents X-axis draw in different color
                if(i == this.x_axis_distance_grid_lines) 
                    ctx.strokeStyle = "#000000";
                else
                    ctx.strokeStyle = "#e9e9e9";
                
                if(i == num_lines_x) {
                    ctx.moveTo(0, this.grid_size*i);
                    ctx.lineTo(this.canvas_width, this.grid_size*i);
                }
                else {
                    ctx.moveTo(0, this.grid_size*i+0.5);
                    ctx.lineTo(this.canvas_width, this.grid_size*i+0.5);
                }
                ctx.stroke();
            }

            // Draw grid lines along Y-axis
            for(i=0; i<=num_lines_y; i++) {
                ctx.beginPath();
                ctx.lineWidth = 1;
                
                // If line represents X-axis draw in different color
                if(i == this.y_axis_distance_grid_lines) 
                    ctx.strokeStyle = "#000000";
                else
                    ctx.strokeStyle = "#e9e9e9";
                
                if(i == num_lines_y) {
                    ctx.moveTo(this.grid_size*i, 0);
                    ctx.lineTo(this.grid_size*i, this.canvas_height);
                }
                else {
                    ctx.moveTo(this.grid_size*i+0.5, 0);
                    ctx.lineTo(this.grid_size*i+0.5, this.canvas_height);
                }
                ctx.stroke();
            }

            // Translate to the new origin. Now Y-axis of the canvas is opposite to the Y-axis of the graph. So the y-coordinate of each element will be negative of the actual
            ctx.translate(this.y_axis_distance_grid_lines*this.grid_size, this.x_axis_distance_grid_lines*this.grid_size);

            // Ticks marks along the positive X-axis
            for(i=1; i<(num_lines_y - this.y_axis_distance_grid_lines); i++) {
                ctx.beginPath();
                ctx.lineWidth = 1;
                ctx.strokeStyle = "#000000";

                // Draw a tick mark 6px long (-3 to 3)
                ctx.moveTo(this.grid_size*i+0.5, -3);
                ctx.lineTo(this.grid_size*i+0.5, 3);
                ctx.stroke();

                // Text value at that point
                if (i % 2 == 0) {
                    ctx.font = '9px Arial';
                    ctx.textAlign = 'start';
                    ctx.fillText(this.x_axis_starting_point.number*i + this.x_axis_starting_point.suffix, this.grid_size*i-2, 15);
                }
            }

            // Ticks marks along the negative X-axis
            for(i=1; i<this.y_axis_distance_grid_lines; i++) {
                ctx.beginPath();
                ctx.lineWidth = 1;
                ctx.strokeStyle = "#000000";

                // Draw a tick mark 6px long (-3 to 3)
                ctx.moveTo(-this.grid_size*i+0.5, -3);
                ctx.lineTo(-this.grid_size*i+0.5, 3);
                ctx.stroke();

                // Text value at that point
                if (i % 2 == 0) {
                    ctx.font = '9px Arial';
                    ctx.textAlign = 'end';
                    ctx.fillText(-this.x_axis_starting_point.number*i + this.x_axis_starting_point.suffix, -this.grid_size*i+3, 15);
                }
            }

            // Ticks marks along the positive Y-axis
            // Positive Y-axis of graph is negative Y-axis of the canvas
            for(i=1; i<(num_lines_x - this.x_axis_distance_grid_lines); i++) {
                ctx.beginPath();
                ctx.lineWidth = 1;
                ctx.strokeStyle = "#000000";

                // Draw a tick mark 6px long (-3 to 3)
                ctx.moveTo(-3, this.grid_size*i+0.5);
                ctx.lineTo(3, this.grid_size*i+0.5);
                ctx.stroke();

                // Text value at that point
                if (i % 2 == 0) {
                    ctx.font = '9px Arial';
                    ctx.textAlign = 'start';
                    ctx.fillText(-this.y_axis_starting_point.number*i + this.y_axis_starting_point.suffix, 8, this.grid_size*i+3);
                }
            }

            // Ticks marks along the negative Y-axis
            // Negative Y-axis of graph is positive Y-axis of the canvas
            for(i=1; i<this.x_axis_distance_grid_lines; i++) {
                ctx.beginPath();
                ctx.lineWidth = 1;
                ctx.strokeStyle = "#000000";

                // Draw a tick mark 6px long (-3 to 3)
                ctx.moveTo(-3, -this.grid_size*i+0.5);
                ctx.lineTo(3, -this.grid_size*i+0.5);
                ctx.stroke();

                // Text value at that point
                if (i % 2 == 0) {
                    ctx.font = '9px Arial';
                    ctx.textAlign = 'start';
                    ctx.fillText(this.y_axis_starting_point.number*i + this.y_axis_starting_point.suffix, 8, -this.grid_size*i+3);
                }
            }

            // draw trucks
            function drawATruck(map, x, y) {
                var truck_width = 60;
                var truck_height = 40;
                var truck_x = x * map.grid_size - Math.floor(truck_width / 2);
                var truck_y = - y * map.grid_size - Math.floor(truck_height / 2);
                ctx.drawImage(truckImg, truck_x, truck_y, truck_width, truck_height);
            }
            // display truck id
            function drawTruckInfo(map, x, y, truck_id, truck_status) {
                var truck_width = 60;
                var truck_height = 40;
                var id_x = x * map.grid_size - Math.floor(truck_width / 6);
                var id_y = - y * map.grid_size + Math.floor(truck_height / 12);
                var status_x = x * map.grid_size - Math.floor(truck_width / 2);
                var status_y = - y * map.grid_size - Math.floor(truck_height / 2);
                console.log(truck_id, id_x, id_y);
                ctx.fillStyle = "white";
                ctx.font = '20px Arial';
                ctx.textAlign = 'center';
                ctx.fillText(truck_id, id_x, id_y);
                
                ctx.fillStyle = "black";
                ctx.font = '25px Arial';
                ctx.textAlign = 'start';
                ctx.fillText(truck_status, status_x, status_y);
            }

            var prev_x = null;
            var prev_y = null;
            // ranging from 0 to 7
            var direction = 0;
            var distance = null;
            for (let i = 0; i < trucks.length; ++i) {
                const truck = trucks[i];
                var curr_x = truck["fields"]["x"];
                var curr_y = truck["fields"]["y"];
                var truck_id = truck["pk"];
                var truck_status = truck["fields"]["status"];
                drawATruck(this, curr_x, curr_y);
                drawTruckInfo(this, curr_x, curr_y, truck_id, truck_status);
            }

            // Translate back to original relation.
            ctx.translate(-this.y_axis_distance_grid_lines * this.grid_size, -this.x_axis_distance_grid_lines * this.grid_size);
        }
    }
}

window.onload = initCanvas();

// var grid_size = 10;
// var x_axis_distance_grid_lines = 20;
// var y_axis_distance_grid_lines = 20;
// var x_axis_starting_point = { number: 1, suffix: '' };
// var y_axis_starting_point = { number: 1, suffix: '' };

// var canvas = document.getElementById("my-canvas");
// var ctx = canvas.getContext("2d");

// var canvas_width = canvas.width;
// var canvas_height = canvas.height;

// var num_lines_x = Math.floor(canvas_height/grid_size);
// var num_lines_y = Math.floor(canvas_width/grid_size);

// // Draw grid lines along X-axis
// for(var i=0; i<=num_lines_x; i++) {
//     ctx.beginPath();
//     ctx.lineWidth = 1;
    
//     // If line represents X-axis draw in different color
//     if(i == x_axis_distance_grid_lines) 
//         ctx.strokeStyle = "#000000";
//     else
//         ctx.strokeStyle = "#e9e9e9";
    
//     if(i == num_lines_x) {
//         ctx.moveTo(0, grid_size*i);
//         ctx.lineTo(canvas_width, grid_size*i);
//     }
//     else {
//         ctx.moveTo(0, grid_size*i+0.5);
//         ctx.lineTo(canvas_width, grid_size*i+0.5);
//     }
//     ctx.stroke();
// }

// // Draw grid lines along Y-axis
// for(i=0; i<=num_lines_y; i++) {
//     ctx.beginPath();
//     ctx.lineWidth = 1;
    
//     // If line represents X-axis draw in different color
//     if(i == y_axis_distance_grid_lines) 
//         ctx.strokeStyle = "#000000";
//     else
//         ctx.strokeStyle = "#e9e9e9";
    
//     if(i == num_lines_y) {
//         ctx.moveTo(grid_size*i, 0);
//         ctx.lineTo(grid_size*i, canvas_height);
//     }
//     else {
//         ctx.moveTo(grid_size*i+0.5, 0);
//         ctx.lineTo(grid_size*i+0.5, canvas_height);
//     }
//     ctx.stroke();
// }

// // Translate to the new origin. Now Y-axis of the canvas is opposite to the Y-axis of the graph. So the y-coordinate of each element will be negative of the actual
// ctx.translate(y_axis_distance_grid_lines*grid_size, x_axis_distance_grid_lines*grid_size);

// // Ticks marks along the positive X-axis
// for(i=1; i<(num_lines_y - y_axis_distance_grid_lines); i++) {
//     ctx.beginPath();
//     ctx.lineWidth = 1;
//     ctx.strokeStyle = "#000000";

//     // Draw a tick mark 6px long (-3 to 3)
//     ctx.moveTo(grid_size*i+0.5, -3);
//     ctx.lineTo(grid_size*i+0.5, 3);
//     ctx.stroke();

//     // Text value at that point
//     if (i % 2 == 0) {
//         ctx.font = '9px Arial';
//         ctx.textAlign = 'start';
//         ctx.fillText(x_axis_starting_point.number*i + x_axis_starting_point.suffix, grid_size*i-2, 15);
//     }
// }

// // Ticks marks along the negative X-axis
// for(i=1; i<y_axis_distance_grid_lines; i++) {
//     ctx.beginPath();
//     ctx.lineWidth = 1;
//     ctx.strokeStyle = "#000000";

//     // Draw a tick mark 6px long (-3 to 3)
//     ctx.moveTo(-grid_size*i+0.5, -3);
//     ctx.lineTo(-grid_size*i+0.5, 3);
//     ctx.stroke();

//     // Text value at that point
//     if (i % 2 == 0) {
//         ctx.font = '9px Arial';
//         ctx.textAlign = 'end';
//         ctx.fillText(-x_axis_starting_point.number*i + x_axis_starting_point.suffix, -grid_size*i+3, 15);
//     }
// }

// // Ticks marks along the positive Y-axis
// // Positive Y-axis of graph is negative Y-axis of the canvas
// for(i=1; i<(num_lines_x - x_axis_distance_grid_lines); i++) {
//     ctx.beginPath();
//     ctx.lineWidth = 1;
//     ctx.strokeStyle = "#000000";

//     // Draw a tick mark 6px long (-3 to 3)
//     ctx.moveTo(-3, grid_size*i+0.5);
//     ctx.lineTo(3, grid_size*i+0.5);
//     ctx.stroke();

//     // Text value at that point
//     if (i % 2 == 0) {
//         ctx.font = '9px Arial';
//         ctx.textAlign = 'start';
//         ctx.fillText(-y_axis_starting_point.number*i + y_axis_starting_point.suffix, 8, grid_size*i+3);
//     }
// }

// // Ticks marks along the negative Y-axis
// // Negative Y-axis of graph is positive Y-axis of the canvas
// for(i=1; i<x_axis_distance_grid_lines; i++) {
//     ctx.beginPath();
//     ctx.lineWidth = 1;
//     ctx.strokeStyle = "#000000";

//     // Draw a tick mark 6px long (-3 to 3)
//     ctx.moveTo(-3, -grid_size*i+0.5);
//     ctx.lineTo(3, -grid_size*i+0.5);
//     ctx.stroke();

//     // Text value at that point
//     if (i % 2 == 0) {
//         ctx.font = '9px Arial';
//         ctx.textAlign = 'start';
//         ctx.fillText(y_axis_starting_point.number*i + y_axis_starting_point.suffix, 8, -grid_size*i+3);
//     }
// }

// var truckImg = document.getElementById("truck");
// let trucks_string = document.getElementById("trucks")
// const trucks = JSON.parse(trucks_string.innerText);
// for (let i = 0; i < trucks.length; ++i) {
//     const truck = trucks[i];
//     drawATruck(ctx, grid_size, truckImg, truck["fields"]["x"], truck["fields"]["y"]);
// }



// function drawATruck(ctx, grid_size, truck, x, y) {
//     var truck_width = 25;
//     var truck_height = 18;
//     var truck_x = x * grid_size - Math.floor(truck_width / 2);
//     var truck_y = - y * grid_size - Math.floor(truck_height / 2);
//     ctx.drawImage(truck, truck_x, truck_y, truck_width, truck_height);
// }