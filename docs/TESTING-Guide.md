# Testing for Topaz 
### Guide Agent
Tested using a modified chat server that sent and recieved messages but did not parse them

##### Command Line
Usage: ` ./guideAgent [-v|-log=raw] [-id=########] teamName playerName GShost GSport`
###### Incorrect parameters
- Invalid Host
    - Expected: Error and exit
    - Output:
    ```
    ./guideAgent teamName playerName cats 12345
    Error: unknown host 'cats'
    ```
- Non-Numerical Port  
    - Expected: Error and exit
    - Output:
    ```
    ./guideAgent teamName playerName flume.cs.dartmouth.edu notaport
    Error: GSport must be a number
    ```
###### Incorrectly ordered parameters (flags come last)
- Flags found after non-flagged parameter
    - Expected: Error and exit
    - Output:
    ```
    ./guideAgent teamName -v playerName cats 12345
    Error: flags must come at the beginning
    ```
###### Invalid Flags
- Unrecognized
    - Expected: Error, but code runs
    - Output:
    ```
    ./guideAgent -unknownflag teamName playerName flume.cs.dartmouth.edu 12345
    Error: flag '-unknownflag' not recognized
    SENT: GA_STATUS|0|55D9CF1E|teamName|playerName|1
    ```
- id flag not a hexcode
    - Expected: Error, but code runs with randomly generated hex id
    - Output:
    ```
    ./guideAgent -id=ZZZZZZZZ teamName playerName flume.cs.dartmouth.edu 12345
    Error: incorrectly formatted -id flag
    SENT: GA_STATUS|0|E61C66C6|teamName|playerName|1
    ```
##### Send Periodic Updates to the Server
After initial GA_STATUS sent and approved by the server,
-  If the user types a zero, sends and prints GA_STATUS
    ```
    
### Field Agent
We tested the field agent through the UI. To ensure that the string for the hex code is properly created, we used a string to print it out. At one point to ensure that messages were properly being formatted, we changed a textlayer label to say status okay. Most of the testing we did was directly on the Pebble UI, and going through many combinations of button presses to make sure that every window worked. Since we could not get the server working, we had trouble testing the server side of the Pebble app, but the UI was thoroughly tested, as were the messenging capabilities.
