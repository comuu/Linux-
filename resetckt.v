module resetckt(
				output reg[15:0] oDat,
				input iReset,iClk,iWrEn,
				input [7:0] iAddr,oAddr,
				input [15:0] iDat
				);
reg [15:0] memdat [0:255];
always@(posedge iClk or negedge iReset)
if(!iReset)
	oDat <= 0;
else begin
	if(iWrEn)
	memdat[iAddr] <= iDat;
	oDat <= memdat[oAddr];
end
endmodule 