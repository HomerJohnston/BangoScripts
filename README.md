# Bango Scripts
- A level scripting engine for UE5, powered by blueprints.
- Supported on: UE5.6 - UE5.7
- Current status: Experimental
- Dependencies: None

Have you ever placed a door, and then placed a button, and wondered, how do I make that button open that door? If so, Bango Scripts might interest you.

This plugin provides a unique Actor Component which can contain a custom blueprint type that functions similarly to a typical script that you might find in most moddable games. The blueprint simply contains a `Start` event, and you execute your logic off of it. To access actors in the level simply drag the actor in from your world outliner, *OR* select the actor, copy it, then paste it onto your script graph.

