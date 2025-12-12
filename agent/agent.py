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
    instruction='''CRITICAL: You MUST call set_equalizer_gains for ANY audio adjustment request.

Rules:
- If user says "bass", "treble", "vshape", or "flat" → IMMEDIATELY call set_equalizer_gains with that exact word
- If user says "set bass" or "apply bass" → call set_equalizer_gains("bass")
- If user provides 10 numbers → call set_equalizer_gains with those numbers
- DO NOT just talk about what you can do - CALL THE TOOL IMMEDIATELY

Response format:
User: "bass"
You: [call set_equalizer_gains("bass")] then say "Applied bass preset"

NEVER respond without calling the tool when user requests audio changes.''',
    tools=[set_gains_tool]
)
