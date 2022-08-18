`timescale 1ns / 1ps

`include "xdefs.vh"
`include "xversat.vh"
`include "xmemdefs.vh"
`include "versat-io.vh"

module VWrite #(
                parameter DATA_W=32
                )
   (
   input                   clk,
   input                   rst,

    input                  run,
    output                 done,

    // Databus interface
    input                  databus_ready,
    output                 databus_valid,
    output[`IO_ADDR_W-1:0] databus_addr,
    input [DATA_W-1:0]     databus_rdata,
    output [DATA_W-1:0]    databus_wdata,
    output [DATA_W/8-1:0]  databus_wstrb,

    // input / output data
    input [DATA_W-1:0]     in0,

    // configurations
   input [`IO_ADDR_W-1:0]  ext_addr,
   input [`MEM_ADDR_W-1:0] int_addr,
   input [`IO_SIZE_W-1:0]  size,
   input [`MEM_ADDR_W-1:0] iterA,
   input [`PERIOD_W-1:0]   perA,
   input [`PERIOD_W-1:0]   dutyA,
   input [`MEM_ADDR_W-1:0] shiftA,
   input [`MEM_ADDR_W-1:0] incrA,
   input                   pingPong,

   input [`MEM_ADDR_W-1:0] iterB,
   input [`PERIOD_W-1:0]   perB,
   input [`PERIOD_W-1:0]   dutyB,
   input [`MEM_ADDR_W-1:0] startB,
   input [`MEM_ADDR_W-1:0] shiftB,
   input [`MEM_ADDR_W-1:0] incrB,
   input [31:0]            delay0, // delayB
   input                   reverseB,
   input                   extB,
   input [`MEM_ADDR_W-1:0] iter2B,
   input [`PERIOD_W-1:0]   per2B,
   input [`MEM_ADDR_W-1:0] shift2B,
   input [`MEM_ADDR_W-1:0] incr2B
   );

   reg last_valid;

   always @(posedge clk,posedge rst)
   begin
      if(rst)
         last_valid <= 0;
      else
         last_valid <= databus_valid;
   end

   wire doneA,doneB;
   assign done = doneA & doneB;

   function [`MEM_ADDR_W-1:0] reverseBits;
      input [`MEM_ADDR_W-1:0]   word;
      integer                   i;

      begin
        for (i=0; i < `MEM_ADDR_W; i=i+1)
          reverseBits[i] = word[`MEM_ADDR_W-1 - i];
      end
   endfunction

   wire [`MEM_ADDR_W-1:0] startA    = `MEM_ADDR_W'd0;
   wire [1:0]             direction = 2'b10;
   wire [`PERIOD_W-1:0]   delayA    = `PERIOD_W'd0;

   // port addresses and enables
   wire [`MEM_ADDR_W-1:0] addrA, addrA_int, addrA_int2;
   wire [`MEM_ADDR_W-1:0] addrB, addrB_int, addrB_int2;

   // data inputs
   wire                   req;
   wire                   rnw;
   wire [DATA_W-1:0]      data_out;

   // mem enables output by addr gen
   wire enA = req;
   wire enB;

   // write enables
   wire wrB = (enB & ~extB); //addrgen on & input on & input isn't address

   wire [DATA_W-1:0]      data_to_wrB = in0;

   wire next;
   wire addrValid;
   wire [`MEM_ADDR_W-1:0] addressA;

   assign databus_wstrb = 4'b1111;

   wire genDone;

   MyAddressGen addrgenA(
      .clk(clk),
      .rst(rst),
      .run(run),
      .next(next),

      //configurations 
      .iterations(iterA),
      .period(perA),
      .duty(dutyA),
      .delay(delayA),
      .start(startA),
      .shift(shiftA),
      .incr(incrA),

      //outputs 
      .valid(addrValid),
      .addr(addressA),
      .done(genDone)
      );

    xaddrgen2 addrgen2B (
                       .clk(clk),
                       .rst(rst),
                       .run(run),
                       .iterations(iterB),
                       .period(perB),
                       .duty(dutyB),
                       .start(startB),
                       .shift(shiftB),
                       .incr(incrB),
                       .delay(delay0[9:0]),
                       .iterations2(iter2B),
                       .period2(per2B),
                       .shift2(shift2B),
                       .incr2(incr2B),
                       .addr(addrB_int),
                       .mem_en(enB),
                       .done(doneB)
                       );

   assign addrA = addrA_int2;
   assign addrB = addrB_int2;

   assign addrA_int2 = addrA_int;
   assign addrB_int2 = reverseB? reverseBits(addrB_int) : addrB_int;
   
   wire read_en;
   wire [`MEM_ADDR_W-1:0] read_addr;
   wire [DATA_W-1:0] read_data;

   MemToIOB #(.DATA_W(DATA_W)) memToIob(
      .ext_addr(ext_addr),

      .read_done(genDone),
      .read_en(addrValid),
      .read_addr(addressA),
      .next(next),

      .mem_en(read_en),
      .mem_addr(read_addr),
      .mem_data(read_data),

      .ready(databus_ready),
      .valid(databus_valid),
      .wdata(databus_wdata),
      .addr(databus_addr),

      .run(run),
      .done(doneA),

      .clk(clk),
      .rst(rst)
   );

   iob_2p_ram #(
                .DATA_W(DATA_W),
                .ADDR_W(`MEM_ADDR_W)
                )
   mem (
        .clk(clk),

        // Reading port
        .r_en(read_en),
        .r_addr(read_addr),
        .r_data(read_data),

        // Writing port
        .w_en(enB & wrB),
        .w_addr(addrB),
        .w_data(data_to_wrB)
        );

endmodule

module MemToIOB #(
      parameter DATA_W = 32
   )(
      input [`IO_ADDR_W-1:0] ext_addr,

      input read_done,
      input read_en,
      input [`MEM_ADDR_W-1:0] read_addr,
      output reg next,

      output reg mem_en,
      output reg [`MEM_ADDR_W-1:0]  mem_addr,
      input [DATA_W-1:0] mem_data,

      input                       ready,
      output reg [`IO_ADDR_W-1:0] addr,
      output reg                  valid,
      output reg [DATA_W-1:0]     wdata,

      input run,
      output reg done,

      input clk,
      input rst
   );

reg [3:0] state;
reg [31:0] counter;

always @(posedge clk,posedge rst)
begin
   if(rst) begin
      state <= 0;
      counter <= 0;
      done <= 0;
   end else if(run) begin
      state <= 4'h1;
      counter <= 0;
      done <= 0;
   end else begin
      case(state)
      4'h0: begin
         state <= 4'h0;
      end
      4'h1: begin
         if(read_en) begin
            state <= 4'h2;
            mem_addr <= read_addr;
         end
      end
      4'h2: begin
         state <= 4'h3;
      end
      4'h3: begin
         state <= 4'h4;
      end
      4'h4: begin
         state <= 4'h5;
         wdata <= mem_data;
         addr <= ext_addr + counter;
         counter <= counter + 32'h4;
      end
      4'h5: begin
         state <= 4'h6;
      end
      4'h6: begin
         if(ready) begin
            if(read_done) begin
               state <= 4'h0;
               done <= 1'b1;
            end else begin
               state <= 4'h1;
            end
         end
      end
      default: begin
         state <= 4'h0;
      end
      endcase
   end
end

always @*
begin
   valid = 1'b0;
   next = 1'b0;
   mem_en = 1'b0;

   case(state)
   4'h2: begin
      mem_en = 1'b1;
   end
   4'h3: begin
      mem_en = 1'b1;
   end
   4'h5: begin
      valid = 1'b1;
   end
   4'h6: begin
      valid = 1'b1;
      if(ready) begin
         if(!read_done) begin
            next = 1'b1;
         end
         next = 1'b1;
         valid = 1'b0;
      end
   end
   default: begin
   end
   endcase
end

endmodule