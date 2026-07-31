"""FastAPI application for the DriveGuard DMS background service."""

from .app import create_app

__all__ = ["create_app"]
