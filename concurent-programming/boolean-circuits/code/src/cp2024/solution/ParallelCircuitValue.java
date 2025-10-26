package cp2024.solution;

import cp2024.circuit.CircuitValue;

public class ParallelCircuitValue implements CircuitValue {

    private final boolean value;

    public ParallelCircuitValue(boolean value) {
        this.value = value;
    }

    @Override
    public boolean getValue() throws InterruptedException {
        return value;
    }

}
