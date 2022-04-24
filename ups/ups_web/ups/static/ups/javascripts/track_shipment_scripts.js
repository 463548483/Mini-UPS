function initCanvas() {
    var canvas = document.getElementById("my-canvas");
    var ctx = canvas.getContext("2d");

    // trucks status from database
    let trucks_string = document.getElementById("trucks")
    const trucks = JSON.parse(trucks_string.innerText);
    let destination_string = document.getElementById("packages")
    const destination = JSON.parse(destination_string.innerText);

    // click button for trucks
    let button = document.getElementById("truck_button");

    // truck image
    var truckImg = new Image();
    // var houseImg =new Image();

    truckImg.onload = function () {
        console.log("Start drawing");
        var map = new Map();
        map.grid_size = 16;

        function animate() {
            ctx.fillStyle = "white";
            ctx.fillRect(0, 0, canvas.width, canvas.height);
            ctx.fillStyle = "black";
            map.render();
        }
        var animateInterval = setInterval(animate, 300);

        // user interactive
        // center a truck
        button.onclick = function () {
            // check if user input the truck number
            let go_to_truck_id = document.getElementById("truck_id_input").value;
            if (go_to_truck_id) {
                // find 
                for (let i = 0; i < trucks.length; ++i) {
                    const truck = trucks[i];
                    var truck_id = truck["pk"];
                    // if truck exists
                    if (truck_id == go_to_truck_id) {
                        var truck_x = truck["fields"]["x"];
                        var truck_y = truck["fields"]["y"];

                        map.y_axis_distance_grid_lines = Math.floor(canvas.width / (2 * map.grid_size)) - truck_x;
                        map.x_axis_distance_grid_lines = Math.floor(canvas.height / (2 * map.grid_size)) + truck_y;
                        break;
                    }
                }
            }
        }

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
        document.addEventListener('keydown', function (event) {
            var key_press = String.fromCharCode(event.keyCode);
            // console.log(key_press);
            if (key_press == "»") {
                // console.log("User wants to scale up");
                // can't scale up too much
                if (map.grid_size <= 64) {
                    map.grid_size *= 2;
                }
            } else if (key_press == "½") {
                // console.log("User wants to scale down");
                // can't scale down too much
                if (map.grid_size >= 16) {
                    map.grid_size /= 2;
                }
            } else if (key_press == "W") {
                // console.log("User wants to move up");
                map.x_axis_distance_grid_lines += 10;
            } else if (key_press == "S") {
                // console.log("User wants to move down");
                map.x_axis_distance_grid_lines -= 10;
            } else if (key_press == "A") {
                // console.log("User wants to move left");
                map.y_axis_distance_grid_lines += 10;
            } else if (key_press == "D") {
                // console.log("User wants to move right");
                map.y_axis_distance_grid_lines -= 10;
            }
        });
    }
    truckImg.src = "https://st.depositphotos.com/62933612/54664/v/380/depositphotos_546643130-stock-illustration-truck-icon-delivery-one-set.jpg?forcejpeg=true";

    // houseImg.onload = function () {
    //     console.log("Start drawing");

    //     function animate() {
    //         ctx.fillStyle = "white";
    //         ctx.fillRect(0, 0, canvas.width, canvas.height);
    //         ctx.fillStyle = "black";
    //         // map.render();
    //     }
    //     var animateInterval = setInterval(animate, 300);
    // }
    // houseImg.src = "https://www.kindpng.com/imgv/oxoibR_home-icon-logo-house-shape-vector-hd-png/";

    function Map() {
        // default value
        // grid size is 16 px
        this.grid_size = 16;
        // location of the x, y main axis
        this.x_axis_distance_grid_lines = 20;
        this.y_axis_distance_grid_lines = 20;
        // axis starting number
        this.x_axis_starting_point = { number: 1, suffix: '' };
        this.y_axis_starting_point = { number: 1, suffix: '' };
        // canvas width and height
        this.canvas_width = canvas.width;
        this.canvas_height = canvas.height;

        this.render = function () {

            var num_lines_x = Math.floor(this.canvas_height / this.grid_size);
            var num_lines_y = Math.floor(this.canvas_width / this.grid_size);

            // Draw grid lines along X-axis
            for (var i = 0; i <= num_lines_x; i++) {
                ctx.beginPath();
                ctx.lineWidth = 1;

                // If line represents X-axis draw in different color
                if (i == this.x_axis_distance_grid_lines)
                    ctx.strokeStyle = "#000000";
                else if (i == num_lines_x && this.x_axis_distance_grid_lines > num_lines_x)
                    ctx.strokeStyle = "#000000";
                else if (i == 0 && this.x_axis_distance_grid_lines < 0)
                    ctx.strokeStyle = "#000000";
                else
                    ctx.strokeStyle = "#e9e9e9";

                if (i == num_lines_x) {
                    ctx.moveTo(0, this.grid_size * i);
                    ctx.lineTo(this.canvas_width, this.grid_size * i);
                }
                else {
                    ctx.moveTo(0, this.grid_size * i + 0.5);
                    ctx.lineTo(this.canvas_width, this.grid_size * i + 0.5);
                }
                ctx.stroke();
            }

            // Draw grid lines along Y-axis
            for (i = 0; i <= num_lines_y; i++) {
                ctx.beginPath();
                ctx.lineWidth = 1;

                // If line represents Y-axis draw in different color
                if (i == this.y_axis_distance_grid_lines)
                    ctx.strokeStyle = "#000000";
                else if (i == num_lines_y && this.y_axis_distance_grid_lines > num_lines_y)
                    ctx.strokeStyle = "#000000";
                else if (i == 0 && this.y_axis_distance_grid_lines < 0)
                    ctx.strokeStyle = "#000000";
                else
                    ctx.strokeStyle = "#e9e9e9";

                if (i == num_lines_y) {
                    ctx.moveTo(this.grid_size * i, 0);
                    ctx.lineTo(this.grid_size * i, this.canvas_height);
                }
                else {
                    ctx.moveTo(this.grid_size * i + 0.5, 0);
                    ctx.lineTo(this.grid_size * i + 0.5, this.canvas_height);
                }
                ctx.stroke();
            }

            // Translate to the new origin. Now Y-axis of the canvas is opposite to the Y-axis of the graph. So the y-coordinate of each element will be negative of the actual
            ctx.translate(this.y_axis_distance_grid_lines * this.grid_size, this.x_axis_distance_grid_lines * this.grid_size);

            // Ticks marks along the positive X-axis
            for (i = 1; i < (num_lines_y - this.y_axis_distance_grid_lines); i++) {
                ctx.beginPath();
                ctx.lineWidth = 1;
                ctx.strokeStyle = "#000000";

                var tick_y;
                if (this.x_axis_distance_grid_lines < 0)
                    tick_y = 3 - this.x_axis_distance_grid_lines * this.grid_size;
                else if (this.x_axis_distance_grid_lines > num_lines_x)
                    tick_y = -3 - (this.x_axis_distance_grid_lines - num_lines_x) * this.grid_size;
                else
                    tick_y = 3;

                // Draw a tick mark 6px long (-3 to 3)
                ctx.moveTo(this.grid_size * i + 0.5, -tick_y);
                ctx.lineTo(this.grid_size * i + 0.5, tick_y);
                ctx.stroke();

                // Text value at that point
                if (i % 2 == 0) {
                    ctx.font = '9px Arial';
                    ctx.textAlign = 'start';
                    var text_y;
                    if (this.x_axis_distance_grid_lines < 0)
                        text_y = 15 - this.x_axis_distance_grid_lines * this.grid_size;
                    else if (this.x_axis_distance_grid_lines > num_lines_x)
                        text_y = -8 - (this.x_axis_distance_grid_lines - num_lines_x) * this.grid_size;
                    else
                        text_y = 15;
                    ctx.fillText(this.x_axis_starting_point.number * i + this.x_axis_starting_point.suffix, this.grid_size * i - 2, text_y);
                }
            }

            // Ticks marks along the negative X-axis
            for (i = 1; i < this.y_axis_distance_grid_lines; i++) {
                ctx.beginPath();
                ctx.lineWidth = 1;
                ctx.strokeStyle = "#000000";

                var tick_y;
                if (this.x_axis_distance_grid_lines < 0)
                    tick_y = 3 - this.x_axis_distance_grid_lines * this.grid_size;
                else if (this.x_axis_distance_grid_lines > num_lines_x)
                    tick_y = -3 - (this.x_axis_distance_grid_lines - num_lines_x) * this.grid_size;
                else
                    tick_y = 3;

                // Draw a tick mark 6px long (-3 to 3)
                ctx.moveTo(-this.grid_size * i + 0.5, -tick_y);
                ctx.lineTo(-this.grid_size * i + 0.5, tick_y);
                ctx.stroke();

                // Text value at that point
                if (i % 2 == 0) {
                    ctx.font = '9px Arial';
                    ctx.textAlign = 'end';
                    var text_y;
                    if (this.x_axis_distance_grid_lines < 0)
                        text_y = 15 - this.x_axis_distance_grid_lines * this.grid_size;
                    else if (this.x_axis_distance_grid_lines > num_lines_x)
                        text_y = -8 - (this.x_axis_distance_grid_lines - num_lines_x) * this.grid_size;
                    else
                        text_y = 15;
                    ctx.fillText(-this.x_axis_starting_point.number * i + this.x_axis_starting_point.suffix, -this.grid_size * i + 3, text_y);
                }
            }

            // Ticks marks along the positive Y-axis
            // Positive Y-axis of graph is negative Y-axis of the canvas
            for (i = 1; i < (num_lines_x - this.x_axis_distance_grid_lines); i++) {
                ctx.beginPath();
                ctx.lineWidth = 1;
                ctx.strokeStyle = "#000000";

                var tick_x;
                if (this.y_axis_distance_grid_lines < 0)
                    tick_x = 3 - this.y_axis_distance_grid_lines * this.grid_size;
                else if (this.y_axis_distance_grid_lines > num_lines_y)
                    tick_x = -3 - (this.y_axis_distance_grid_lines - num_lines_y) * this.grid_size;
                else
                    tick_x = 3;

                // Draw a tick mark 6px long (-3 to 3)
                ctx.moveTo(-tick_x, this.grid_size * i + 0.5);
                ctx.lineTo(tick_x, this.grid_size * i + 0.5);
                ctx.stroke();

                // Text value at that point
                if (i % 2 == 0) {
                    ctx.font = '10px Arial';
                    ctx.textAlign = 'start';
                    var text_x;
                    if (this.y_axis_distance_grid_lines < 0)
                        text_x = 8 - this.y_axis_distance_grid_lines * this.grid_size;
                    else if (this.y_axis_distance_grid_lines > num_lines_y)
                        text_x = -20 - (this.y_axis_distance_grid_lines - num_lines_y) * this.grid_size;
                    else
                        text_x = 8;
                    ctx.fillText(-this.y_axis_starting_point.number * i + this.y_axis_starting_point.suffix, text_x, this.grid_size * i + 3);
                }
            }

            // Ticks marks along the negative Y-axis
            // Negative Y-axis of graph is positive Y-axis of the canvas
            for (i = 1; i < this.x_axis_distance_grid_lines; i++) {
                ctx.beginPath();
                ctx.lineWidth = 1;
                ctx.strokeStyle = "#000000";

                var tick_x;
                if (this.y_axis_distance_grid_lines < 0)
                    tick_x = 3 - this.y_axis_distance_grid_lines * this.grid_size;
                else if (this.y_axis_distance_grid_lines > num_lines_y)
                    tick_x = -3 - (this.y_axis_distance_grid_lines - num_lines_y) * this.grid_size;
                else
                    tick_x = 3;
                // Draw a tick mark 6px long (-3 to 3)
                ctx.moveTo(-tick_x, -this.grid_size * i + 0.5);
                ctx.lineTo(tick_x, -this.grid_size * i + 0.5);
                ctx.stroke();

                // Text value at that point
                if (i % 2 == 0) {
                    ctx.font = '9px Arial';
                    ctx.textAlign = 'start';
                    var text_x;
                    if (this.y_axis_distance_grid_lines < 0)
                        text_x = 8 - this.y_axis_distance_grid_lines * this.grid_size;
                    else if (this.y_axis_distance_grid_lines > num_lines_y)
                        text_x = -20 - (this.y_axis_distance_grid_lines - num_lines_y) * this.grid_size;
                    else
                        text_x = 8;
                    ctx.fillText(this.y_axis_starting_point.number * i + this.y_axis_starting_point.suffix, text_x, -this.grid_size * i + 3);
                }
            }

            // draw trucks
            function drawATruck(map, x, y) {
                var truck_width = 60;
                var truck_height = 50;
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
                // console.log(truck_id, id_x, id_y);
                ctx.fillStyle = "white";
                ctx.font = '20px Arial';
                ctx.textAlign = 'center';
                ctx.fillText(truck_id, id_x, id_y);

                ctx.fillStyle = "black";
                ctx.font = '25px Arial';
                ctx.textAlign = 'start';
                ctx.fillText(truck_status, status_x, status_y);
            }

            // draw destination
            function drawAHouse(map, x, y) {
                var house_width = 60;
                var house_height = 50;
                var house_x = x * map.grid_size - Math.floor(house_width / 2);
                var house_y = - y * map.grid_size - Math.floor(house_height / 2);
                ctx.drawImage(houseImg, house_x, house_y, house_width, house_height);
            }
            

            for (let i = 0; i < trucks.length; ++i) {
                const truck = trucks[i];
                var curr_x = truck["fields"]["x"];
                var curr_y = truck["fields"]["y"];
                var truck_id = truck["pk"];
                var truck_status = truck["fields"]["status"];
                drawATruck(this, curr_x, curr_y);
                drawTruckInfo(this, curr_x, curr_y, truck_id, truck_status);
            }

            // for (let i = 0; i < destination.length; ++i) {
            //     const dest = destination[i];
            //     var curr_x = dest["fields"]["destx"];
            //     var curr_y = dest["fields"]["desty"];
            //     drawAHouse(this, curr_x, curr_y);
            // }

            // Translate back to original relation.
            ctx.translate(-this.y_axis_distance_grid_lines * this.grid_size, -this.x_axis_distance_grid_lines * this.grid_size);
        }
    }
}

window.onload = initCanvas();


