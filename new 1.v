primitive multiplexer(o,a,b,s);
output o;
input s,a,b;
table//由端口输入顺序决定
//a b s : o
  0 ? 1 : 0;
endtable
endprimitive

primitive

output q;
reg q;
input clock,data;
initial q = 1'b1;
table

endtable
endprimitive

module select(a,b,sl);
input [1:0]sl;
output 
endmodule 
module a(clk,)
module regwrite(rout, clk, in, ctrl);
output reg[3:0]rout;
input clk,in;
input [3:0]ctrl;
always@(posedge clk)begin
if(ctrl[0])
end
