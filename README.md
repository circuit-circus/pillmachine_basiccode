# The Basic Code for the Museion Pill Machine project

A repository for the machines of the Medicinal Museion Pill Machine project with Mogens Jacobsen and Circuit Circus. 

Each machine should be created with a new branch with a name that is prefixed with `machine_`.

The `develop` branch is the main code, i.e. the Ethernet and RFID code that is common to all machines. If changes occur here, they should be merged into each individual machine's branch.

## How to add individual machines

1. Branch out from the `develop` branch with a name that starts with `machine_`
3. Add your variables after the comment that reads `// Define machine individual variables here`
4. Add your setups after the comment that reads `// Machine individual setups go here`
5. Add your loop flow in the `UI()` function, and make sure to set `userval` in the end of the loop
6. Add any custom functions you may need after the comment that reads `// Custom functions for individual machines go here`

## Debugging

If you don't have an RFID reader and/or Ethernet Shield available, you can set `cardPresent` and `isDebugging` to `true` in order to make sure the Arduino enters the `UI()` loop.

## Example

See the branch named `example` for more info.