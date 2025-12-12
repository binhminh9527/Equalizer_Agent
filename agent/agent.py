from google.adk.agents.llm_agent import Agent
from google.adk.tools import FunctionTool
import subprocess
import os
from datetime import datetime

# Simple log helper to see when the tool runs
LOG_PATH = os.path.join(os.path.dirname(__file__), "set_gains_tool.log")

def _log_tool(message: str) -> None:
    try:
        timestamp = datetime.utcnow().isoformat()
        with open(LOG_PATH, "a", encoding="utf-8") as f:
            f.write(f"[{timestamp}] {message}\n")
    except Exception:
        pass

# API key can be passed via environment variable GOOGLE_API_KEY
# If not set, the agent will fail at runtime when trying to call the model

# --- Define the Agent's Tool ---
def set_equalizer_gains(gains: str) -> str:
    """
    Set equalizer band gains.
    Args:
        gains: Space-separated 10 gain values (e.g., "0 3 -2 0 5 0 -3 2 0 1") 
               or preset name (flat, bass, treble, vshape)
    Returns:
        Result of setting gains
    """
    _log_tool(f"invoked with gains='{gains}'")
    try:
        # Check if it's a preset
        presets = ["flat", "bass", "treble", "vshape"]
        if gains.lower() in presets:
            cmd = ["python3", "test_set_gains.py", "--preset", gains.lower()]
        else:
            # Parse as individual gains
            gain_list = gains.split()
            if len(gain_list) != 10:
                msg = "ERROR: Need exactly 10 gains or a preset name (flat/bass/treble/vshape)"
                _log_tool(msg)
                return msg
            cmd = ["python3", "test_set_gains.py"] + gain_list
        
        result = subprocess.run(cmd, cwd="/home/minhnb/Equalizer_Agent/agent", capture_output=True, text=True, timeout=5)
        if result.returncode == 0:
            msg = f"✓ Gains set successfully. {result.stdout.strip()}"
            _log_tool(msg)
            return msg
        else:
            msg = f"✗ Error: {result.stderr.strip()}"
            _log_tool(msg)
            return msg
    except Exception as e:
        msg = f"Error: {str(e)}"
        _log_tool(msg)
        return msg

# --- Initialize Agent ---

# Tools for the agent
set_gains_tool = FunctionTool(set_equalizer_gains)

# Main agent
root_agent = Agent(
    model='gemini-2.5-flash',
    name='EqualizerAssistant',
    description='Audio equalizer assistant that can adjust audio equalizer settings',
    instruction='''You are a helpful audio equalizer assistant that actively helps users adjust their audio settings.

IMPORTANT: When a user wants to change audio settings, you MUST use the set_equalizer_gains tool to apply the changes.

You can help users by:
1. Listening to their preferences (more bass, clearer vocals, less treble, etc.)
2. Recommending and APPLYING equalizer adjustments
3. Using presets: "bass", "treble", "vshape", or "flat"
4. Creating custom settings with 10 gain values for these bands:
   - 31Hz, 62Hz, 125Hz, 250Hz, 500Hz, 1kHz, 2kHz, 4kHz, 8kHz, 16kHz

When applying settings:
- For presets: Call set_equalizer_gains with preset name (e.g., "bass")
- For custom: Call set_equalizer_gains with 10 space-separated dB values (e.g., "0 3 -2 0 5 0 -3 2 0 1")
- Values range from -30 to +30 dB

Examples of when to use the tool:
- User says "make it bassier" → Use set_equalizer_gains with "bass" or custom values like "6 5 3 0 0 0 0 0 0 0"
- User says "bass preset" → Use set_equalizer_gains with "bass"
- User says "increase 31hz" → Use set_equalizer_gains with custom values starting with higher first value
- User says "tune something random" → Create random but reasonable values and apply them

Always explain (very shortly) what you're doing BEFORE calling the tool, then confirm the changes after.''',
    tools=[set_gains_tool]
)
