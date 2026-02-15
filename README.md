# Bango Scripts

### THE PROBLEM

- Have you ever placed a trigger volume and wondered *"how can I spawn three enemies at some markers, then turn on those lights, and start playing some boss music after 2 seconds? What if I want to spawn different enemies based on the player's skill setting?"*
- Many teams will build some sort of "Actions" system which involves arrays full of "Action" entries. This is clanky, difficult to visualize and work with, and can require a lot of effort to build new entry types, even for basic things.

### THE SOLUTION

- What if you could write an *instanced* blueprint event directly onto an actor placed in a level?
- What if you could reference other actors in the level with simple, easy-to-use nodes?
- What if you had more blueprint nodes that helped with actual level scripting, and not just O.O.P. coding?

### SUMMARY INFO
> [!NOTE]
> - Intended to execute small, short-lived scripted events.
> - Supported on: UE5.6 - UE5.7
> - Current status: ⚠️**Experimental**⚠️
 
> [!WARNING]
> - *Runtime serialization*: Bango uses Blueprint. Blueprint graph execution state is not serializable; you cannot save/restore a script back to its middle of operation (you might want to use a different system for that, like Flow Graph).
> - *Untested in multiplayer*: this plugin is currently being used for single-player projects; it may work fine, but is untested in multiplayer.

> [!CAUTION]
> - This plugin is experimental; be sure to visit the [Issues](https://github.com/HomerJohnston/BangoScripts/issues) page for any known problems.

&nbsp;

### DEMONSTRATION

https://github.com/user-attachments/assets/f2b23127-251b-479e-bb31-d9f940fdac69

The level above contains:
### GENERAL ACTORS:
- Two doors, each with a simple "open" event. The animation, light, and sound is all handled in the door blueprint.
- Several lights, each with a simple "turn on" event. Like the door, the animation and sound of the lights is handled in the light blueprint.
- Two loudspeakers, just dummy actors for using PlaySoundAtLocation.

### TRIGGER ACTORS, WITH BANGO SCRIPTS:
- One blank actor with a script, set to auto-run. This starts the music and announcer voices coming from two loudspeakers.
- One pressure-plate trigger controlling the first door. This contains an overlap collider box and two Bango Script components (Activate/Deactivate). The Activate script runs on BeginOverlap, and vice versa.
- Two simple sphere collider triggers, the first turning on the hallway lights and the second opening the last door. These contain an overlap sphere and a Bango Script component. The script component is ran on the trigger's BeginOverlap event. The trigger is coded to self-destruct, to prevent repeat activations.

The trigger actors are NOT related to Bango Scripts. You will need to supply your own reusable trigger classes. More info in the wiki.

&nbsp;

Here's what the scripts for this scene look like, in order of appearance:

&nbsp;

<img width="1910" height="1068" alt="image" src="https://github.com/user-attachments/assets/739bfc2a-4b73-423b-90a7-c6e27792e438" />

<p align="center"><i>The startup script. This plays PA system audio and starts music for the scene.</i></p>

&nbsp;

<img width="1908" height="1069" alt="image" src="https://github.com/user-attachments/assets/f38f90f5-57d1-4bd1-9d0e-4a7a9bc1e33b" />

<p align="center"><i>The pressure plate's "Activate" script. This opens the door. It also has a separate "Deactivate" script to close the door.</i></p>

&nbsp;

<img width="1904" height="1076" alt="image" src="https://github.com/user-attachments/assets/7eed9bf3-e41c-42d9-8f02-ccc402dd0f0c" />

<p align="center"><i>The first sphere-volume trigger. This turns on the hallway lights after the avatar walks through the door.</i></p>

&nbsp;

<img width="1909" height="1077" alt="image" src="https://github.com/user-attachments/assets/6ed47123-a415-4f21-8102-7a48c123d979" />

<p align="center"><i>The second sphere-volume trigger. This opens the second door as the avatar gets close to it.</i></p>

&nbsp;

### GETTING STARTED

Go to the wiki to try out Bango Scripts.
