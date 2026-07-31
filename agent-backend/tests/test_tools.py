from app.tools import TOOLS


def test_tool_names_are_unique() -> None:
    names = [item["function"]["name"] for item in TOOLS]
    assert len(names) == len(set(names))


def test_all_tool_parameters_are_objects() -> None:
    for item in TOOLS:
        assert item["function"]["parameters"]["type"] == "object"
