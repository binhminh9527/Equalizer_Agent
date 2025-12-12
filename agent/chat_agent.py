#!/usr/bin/env python3
"""Chat bridge for the Equalizer agent (TCP port 5555)."""

import asyncio
import json
import logging
import socket
import threading
from pathlib import Path

from dotenv import load_dotenv

from agent import root_agent
from google.adk.apps.app import App
from google.adk.artifacts.in_memory_artifact_service import InMemoryArtifactService
from google.adk.auth.credential_service.in_memory_credential_service import InMemoryCredentialService
from google.adk.memory.in_memory_memory_service import InMemoryMemoryService
from google.adk.runners import Runner
from google.adk.sessions import InMemorySessionService
from google.adk.utils.context_utils import Aclosing
from google.genai import types as genai_types



# Load environment variables from .env
load_dotenv(Path(__file__).parent / '.env')


logging.basicConfig(level=logging.INFO, format='[%(asctime)s] %(message)s')
log = logging.getLogger("chat_agent")


class ChatAgent:
    def __init__(self, host: str = 'localhost', port: int = 5555) -> None:
        self.host = host
        self.port = port
        self.server = None
        self.running = True
        self.session_service = None
        self.session = None
        self.runner = None

    async def _invoke_agent_async(self, user_input: str) -> str:
        """Invoke the agent via Runner so session history persists."""
        log.info("[agent] Invoking with user_input='%s'", user_input)

        content = genai_types.Content(role='user', parts=[genai_types.Part(text=user_input)])

        full_response = ""
        event_count = 0

        async with Aclosing(
            self.runner.run_async(
                user_id=self.session.user_id,
                session_id=self.session.id,
                new_message=content,
            )
        ) as agen:
            async for event in agen:
                event_count += 1
                if event.content and event.content.parts:
                    for part in event.content.parts:
                        if getattr(part, "text", None):
                            full_response += part.text

        log.info("Total events: %s, Response length: %s", event_count, len(full_response))
        return full_response if full_response else "I couldn't generate a response."

    async def process_message_async(self, user_input: str) -> str:
        """Invoke the AI agent for one user turn."""
        try:
            return await self._invoke_agent_async(user_input)
        except Exception as e:
            log.exception("Agent error: %s", e)
            return f"Error: {str(e)[:200]}"

    def process_message(self, user_input: str) -> str:
        """Sync wrapper for agent invocation (per client thread)."""
        loop = asyncio.new_event_loop()
        try:
            asyncio.set_event_loop(loop)
            return loop.run_until_complete(self.process_message_async(user_input))
        finally:
            loop.close()

    def handle_client(self, conn, addr) -> None:
        """Handle one TCP client connection."""
        log.info("Client connected: %s", addr)
        try:
            while self.running:
                data = conn.recv(1024)
                if not data:
                    break

                message = data.decode('utf-8').strip()
                if not message:
                    continue
                log.info("[%s] Received: %s", addr, message)

                response = self.process_message(message)
                log.info("[%s] Responding: %s...", addr, response[:100])

                conn.sendall(json.dumps({"response": response}).encode('utf-8'))
        except Exception as e:
            log.exception("Client error: %s", e)
        finally:
            conn.close()
            log.info("Client disconnected: %s", addr)

    def start(self) -> None:
        """Start the TCP server and initialize session with initial greeting."""
        loop = asyncio.new_event_loop()
        asyncio.set_event_loop(loop)
        
        self.session_service = InMemorySessionService()
        self.session = loop.run_until_complete(
            self.session_service.create_session(app_name='EQ_Chat', user_id='chat_user')
        )

        app = App(name='EQ_Chat', root_agent=root_agent)
        self.runner = Runner(
            app=app,
            artifact_service=InMemoryArtifactService(),
            session_service=self.session_service,
            memory_service=InMemoryMemoryService(),
            credential_service=InMemoryCredentialService(),
        )

        loop.close()

        self.server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        self.server.bind((self.host, self.port))
        self.server.listen(5)

        log.info("Chat Agent listening on %s:%s", self.host, self.port)
        log.info("AI Agent ready with SetEqualizerGains tool!")

        try:
            while self.running:
                conn, addr = self.server.accept()
                thread = threading.Thread(target=self.handle_client, args=(conn, addr), daemon=True)
                thread.start()
        except KeyboardInterrupt:
            log.info("Shutting down...")
        finally:
            self.server.close()


if __name__ == '__main__':
    ChatAgent().start()