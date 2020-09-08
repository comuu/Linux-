module randomlogic(
		output reg [7:0] Out,
		input [7:0] A,B,C,
		input clk,
		input Cond1,Cond2);
	always@(posedge clk)
	if(Cond1)
	Out <= A;
	else if(Cond2 && (C < 8))
	Out <= B;
	else 
	Out <= C;
endmodule 