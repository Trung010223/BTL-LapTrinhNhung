import os
from typing import Generator

from sqlmodel import SQLModel, Session, create_engine

DATABASE_URL = os.getenv("DATABASE_URL", "sqlite:///./robot_arm.db")

engine_kwargs = {}
if DATABASE_URL.startswith("sqlite"):
    engine_kwargs["connect_args"] = {"check_same_thread": False}
else:
    # Keep MySQL/MariaDB connections healthy in long-running API process.
    engine_kwargs["pool_pre_ping"] = True

engine = create_engine(
    DATABASE_URL,
    **engine_kwargs,
)


def create_db_and_tables():
    SQLModel.metadata.create_all(engine)


def get_session() -> Generator:
    with Session(engine) as session:
        yield session


get_db = get_session
