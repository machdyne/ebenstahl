/*
 * Ebenstahl Case
 * Copyright (c) 2025 Lone Dynamics Corporation. All rights reserved.
 *
 * required hardware:
 *  - 4 x M3 x 30mm countersunk bolts
 *  - 4 x M3 nuts
 *
 */

$fn = 100;

board_width = 50;
board_thickness = 1.5;
board_length = 100;
board_height = 1.6;
board_spacing = 2;

wall = 2;

top_height = 25;
bottom_height = 6;

translate([wall,wall,4]) ssz_board();

color([1,0,0]) translate([0,0,5])
    ssz_case_top();

color([1,0,0]) translate([0,0,0])
	ssz_case_bottom();

translate([0,0,15])
	ssz_case_slide();

module ssz_board() {
	
	difference() {
		color([0,0.5,0])
			roundedcube(board_width,board_length,board_thickness,4);
		translate([4, 4, -1]) cylinder(d=3.2, h=10);
		translate([4, 100-4, -1]) cylinder(d=3.2, h=10);
		translate([50-4, 4, -1]) cylinder(d=3.2, h=10);
		translate([50-4, 100-4, -1]) cylinder(d=3.2, h=10);
	}	
	
}

module ssz_case_top() {

	translate([54,75,19])
		rotate([90,0,90])
			translate([0,-10,0])
				linear_extrude(1)
					text("EBENSTAHL", size=5, halign="center",
						font="Lato Black");

	// supports
	translate([wall, wall, 0]) {
	}
	
	difference() {
				
		color([0.5,0.5,0.5])
			roundedcube(board_width+(wall*2),board_length+(wall*2),top_height,4);

		// cutouts
			
		translate([2,8.5,-2])
			roundedcube(board_width,board_length-13,25,5);

		translate([8.5,2,-2])
			roundedcube(board_width-13,board_length,25,5);			
	
		translate([wall, wall, 0]) {

			// vents
			//translate([(52/2)-5,-5,10]) rotate([0,0,90]) cube([15,1,20]);
			//translate([(52/2),-5,10]) rotate([0,0,90]) cube([15,1,20]);
			//translate([(52/2)+5,-5,10]) rotate([0,0,90]) cube([15,1,20]);
			
			// USBC
			translate([-10,15-(9.5/2),0]) cube([30,9.5,3.5]);

			// LED vent
   			translate([37.5,25,5]) rotate([90,0,0]) cylinder(d=1.5, h=100);
            translate([37.5,25,7.5]) rotate([90,0,0]) cylinder(d=1.5, h=100);
            translate([37.5,25,10]) rotate([90,0,0]) cylinder(d=1.5, h=100);

			// BOOTSEL
			translate([30,15-(8/2),-1]) cube([30,8,7.5+1]);

			// WP
			translate([50-14.2425-(9/2),99,-1]) cube([9,10,2.5+1]);
				
			// bolt holes
			translate([4, 4, -15]) cylinder(d=3.5, h=40);
			translate([4, 100-4, -15]) cylinder(d=3.5, h=40);
			translate([50-4, 4, -14]) cylinder(d=3.5, h=40);
			translate([50-4, 100-4, -14]) cylinder(d=3.5, h=40);

			// flush mount bolt holes
			translate([4, 4, top_height-1.75]) cylinder(d=6, h=4);
			translate([4, 100-4, top_height-1.75]) cylinder(d=6, h=4);
			translate([50-4, 4, top_height-1.75]) cylinder(d=6, h=4);
			translate([50-4, 100-4, top_height-1.75]) cylinder(d=6, h=4);


		}
		
	}	
}

module ssz_case_bottom() {
	
	difference() {
		color([0.5,0.5,0.5])
			roundedcube(board_width+(wall*2),board_length+(wall*2),bottom_height,4);
		
		// cutouts
		translate([3.5,10,1.5])
			roundedcube(board_width-3,board_length-17,10,5);
				
		translate([10.5,3,1.5])
			roundedcube(board_width-17.5,board_length-2,10,5);

		translate([wall, wall, 0]) {
			
		// board cutout
		translate([0,0,bottom_height-board_height])
			roundedcube(board_width+0.2,board_length+0.2,board_height+1,2.5);

		// USBC
		translate([0,17.275-(10/2),1.5]) cube([30,10,3.5]);

		// BOOTSEL
		translate([20.2,17.275-(10/2),1.5]) cube([30,10,7.5+1]);
            
		// bolt holes
		translate([4, 4, -11]) cylinder(d=3.2, h=25);
		translate([4, 100-4, -11]) cylinder(d=3.2, h=25);
		translate([50-4, 4, -11]) cylinder(d=3.2, h=25);
		translate([50-4, 100-4, -11]) cylinder(d=3.2, h=25);

		// nut holes
		translate([4, 4, -1]) cylinder(d=7, h=4, $fn=6);
		translate([4, 100-4, -1]) cylinder(d=7, h=4, $fn=6);
		translate([50-4, 4, -1]) cylinder(d=7, h=4, $fn=6);
		translate([50-4, 100-4, -1]) cylinder(d=7, h=4, $fn=6);

		}
		
	}	
}

// https://gist.github.com/tinkerology/ae257c5340a33ee2f149ff3ae97d9826
module roundedcube(xx, yy, height, radius)
{
    translate([0,0,height/2])
    hull()
    {
        translate([radius,radius,0])
        cylinder(height,radius,radius,true);

        translate([xx-radius,radius,0])
        cylinder(height,radius,radius,true);

        translate([xx-radius,yy-radius,0])
        cylinder(height,radius,radius,true);

        translate([radius,yy-radius,0])
        cylinder(height,radius,radius,true);
    }
}
