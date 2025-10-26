package cp2024.circuit;
/** A circuit node of type true false jest od razu wartoscia nie ma typu */
public abstract non-sealed class LeafNode extends CircuitNode {
    protected LeafNode() {
        super(NodeType.LEAF, new CircuitNode[0]);
    }

    abstract public boolean getValue() throws InterruptedException;
}
