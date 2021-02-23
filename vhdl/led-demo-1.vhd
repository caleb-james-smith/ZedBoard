----------------------------------------------------------------------------------
-- Company:
-- Engineer:
--
-- Create Date: 02/20/2021 12:13:36 PM
-- Design Name:
-- Module Name: led_2 - Behavioral
-- Project Name:
-- Target Devices:
-- Tool Versions:
-- Description:
--
-- Dependencies:
--
-- Revision:
-- Revision 0.01 - File Created
-- Additional Comments:
--
----------------------------------------------------------------------------------


library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
--use IEEE.NUMERIC_STD.ALL;

-- Uncomment the following library declaration if instantiating
-- any Xilinx leaf cells in this code.
--library UNISIM;
--use UNISIM.VComponents.all;

entity led_2 is
    Port ( GCLK : in  STD_LOGIC;
           LD0  : out STD_LOGIC);
end led_2;

architecture Behavioral of led_2 is
    signal clk_counter : natural range 0 to 50000000 := 0;
    signal blinker     : STD_LOGIC                   := '0';
begin
    process(GCLK)
    begin
        if rising_edge(GCLK) then
            clk_counter <= clk_counter + 1;
            if clk_counter >= 50000000 then
                blinker <= not blinker;
                clk_counter <= 0;
            end if;
        end if;
    end process;
    LD0 <= blinker;
end Behavioral;
