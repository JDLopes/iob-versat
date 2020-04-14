`timescale 1ns / 1ps
`include "xversat.vh"

module  xinmux (
                input [`N_W:0]         sel,
                input [`DATABUS_W-1:0] data_in,
                output [`DATA_W-1:0]   data_out
                );

   always @ (*) begin
      integer 		i;
      for (i=0; i < 2*`N; i = i+1)
	if (sel == i)
	  data_out = data_in[`DATABUS_W-`DATA_W*i -1 -: `DATA_W];
   end

endmodule
