package cp2024.solution;

import cp2024.circuit.*;
import cp2024.demo.BrokenCircuitValue;

import java.util.ArrayList;
import java.util.concurrent.*;


public class ParallelCircuitSolver implements CircuitSolver {

    private static final ExecutorService cachedThreadPool = Executors.newCachedThreadPool();

    private boolean allowComputations = true;

    @Override
    public CircuitValue solve(Circuit c) {
        if (!allowComputations) {
            return new BrokenCircuitValue();
        }

        boolean value;

        try {
            value = parallelSolve(c.getRoot());
        }
        catch (InterruptedException e) {
            return new BrokenCircuitValue();
        }

        return new ParallelCircuitValue(value);
    }

    @Override
    public void stop() {
        allowComputations = false;
        cachedThreadPool.shutdownNow();
    }

    private boolean parallelSolve(CircuitNode node) throws InterruptedException {
        if (Thread.currentThread().isInterrupted()) {
            throw new InterruptedException();
        }

        if (node.getType() == NodeType.LEAF) {
            return ((LeafNode) node).getValue();
        }

        CircuitNode[] args = node.getArgs();


        return switch (node.getType()) {
            case IF -> solveIF(args);
            case AND -> solveAND(args);
            case OR -> solveOR(args);
            case GT -> solveGT(args, ((ThresholdNode) node).getThreshold());
            case LT -> solveLT(args, ((ThresholdNode) node).getThreshold());
            case NOT -> solveNOT(args);
            default -> throw new RuntimeException("Illegal type: " + node.getType());
        };
    }


    private boolean solveNOT(CircuitNode[] args) throws InterruptedException {
        ExecutorCompletionService<Boolean> completionService = new ExecutorCompletionService<>(cachedThreadPool);

        try {
            Future<Boolean> future = completionService.submit(() -> parallelSolve(args[0]));
            if (Thread.currentThread().isInterrupted()) {
                future.cancel(true);
                throw new InterruptedException();
            }
            return !future.get();
        } catch (InterruptedException | ExecutionException e) {
            Thread.currentThread().interrupt();
            throw new InterruptedException();
        }
    }


    private boolean solveIF(CircuitNode[] args) throws InterruptedException {
        ExecutorCompletionService<Boolean> completionService = new ExecutorCompletionService<>(cachedThreadPool);

        Future<Boolean> conditionFuture = completionService.submit(() -> parallelSolve(args[0]));
        Future<Boolean> ifTrueFuture = completionService.submit(() -> parallelSolve(args[1]));
        Future<Boolean> ifFalseFuture = completionService.submit(() -> parallelSolve(args[2]));

        try {
            boolean condition = conditionFuture.get();
            boolean result;
            if (condition) {
                result = ifTrueFuture.get();
                ifFalseFuture.cancel(true);
            } else {
                result = ifFalseFuture.get();
                ifTrueFuture.cancel(true);
            }
            return result;
        } catch (InterruptedException | ExecutionException e) {
            Thread.currentThread().interrupt();
            throw new InterruptedException();
        }
    }


    // True gdy trueNumber <= threshold - 1 czyli true number < treshold
    private boolean solveLT(CircuitNode[] args, int threshold) throws InterruptedException {
        if (threshold <= 0) {
            return false;
        } else if (threshold > args.length) {
            return true;
        }
        // Completion service - pomoc przy leniwym podejściu
        ExecutorCompletionService<Boolean> completionService = new ExecutorCompletionService<>(cachedThreadPool);

        ArrayList<Future<Boolean>> futuresList = new ArrayList<>();

        for (CircuitNode arg : args) {
            Future<Boolean> future =  completionService.submit(() -> parallelSolve(arg));
            futuresList.add(future);
        }

        int trueNumber = 0;

        for (int i = 0; i < args.length; i++) {
            if (Thread.currentThread().isInterrupted()) {
                cancelFutures(futuresList);
                Thread.currentThread().interrupt();
                throw new InterruptedException();
            }

            try {
                Future<Boolean> currentFuture = completionService.take();

                if (currentFuture.get()) {
                    trueNumber++;
                }

                if (trueNumber >= threshold) {
                    cancelFutures(futuresList);
                    return false;
                }

                // Zostało mniej węzłów + zliczonych niż treshold
                if (args.length - i + trueNumber < threshold) {
                    cancelFutures(futuresList);
                    return true;
                }
            }
            catch (InterruptedException | ExecutionException e) {
                cancelFutures(futuresList);
                Thread.currentThread().interrupt();
                throw new InterruptedException();
            }
        }
        return true;
    }

    // Zwraca true gdy trueNumber >= threshold + 1 czyli trueNumber > treshold
    private boolean solveGT(CircuitNode[] args, int threshold) throws InterruptedException {
        if (threshold < 0) {
            return true;
        } else if (threshold >= args.length) {
            return false;
        }
        // Completion service - pomoc przy leniwym podejściu
        ExecutorCompletionService<Boolean> completionService = new ExecutorCompletionService<>(cachedThreadPool);

        ArrayList<Future<Boolean>> futuresList = new ArrayList<>();

        for (CircuitNode arg : args) {
            Future<Boolean> future =  completionService.submit(() -> parallelSolve(arg));
            futuresList.add(future);
        }

        int trueNumber = 0;

        for (int i = 0; i < args.length; i++) {
            if (Thread.currentThread().isInterrupted()) {
                cancelFutures(futuresList);
                Thread.currentThread().interrupt();
                throw new InterruptedException();
            }

            try {
                Future<Boolean> currentFuture = completionService.take();

                if (currentFuture.get()) {
                    trueNumber++;
                }

                if (trueNumber > threshold) {
                    cancelFutures(futuresList);
                    return true;
                }

                // Brak możliwości uzyskania trueNo > treshold
                if (args.length - i < threshold - trueNumber + 1) {
                    cancelFutures(futuresList);
                    return false;
                }
            }
            catch (InterruptedException | ExecutionException e) {
                cancelFutures(futuresList);
                Thread.currentThread().interrupt();
                throw new InterruptedException();
            }
        }
        return false;
    }


    private boolean solveOR(CircuitNode[] args) throws InterruptedException {
        // Completion service - pomoc przy leniwym podejściu
        ExecutorCompletionService<Boolean> completionService = new ExecutorCompletionService<>(cachedThreadPool);

        ArrayList<Future<Boolean>> futuresList = new ArrayList<>();

        for (CircuitNode arg : args) {
            Future<Boolean> future =  completionService.submit(() -> parallelSolve(arg));
            futuresList.add(future);
        }

        for (int i = 0; i < args.length; i++) {
            if (Thread.currentThread().isInterrupted()) {
                cancelFutures(futuresList);
                Thread.currentThread().interrupt();
                throw new InterruptedException();
            }

            try {
                Future<Boolean> currentFuture = completionService.take();

                if (currentFuture.get()) {
                    cancelFutures(futuresList);
                    return true;
                }
            }
            catch (InterruptedException | ExecutionException e) {
                cancelFutures(futuresList);
                Thread.currentThread().interrupt();
                throw new InterruptedException();
            }
        }
        return false;
    }

    private boolean solveAND(CircuitNode[] args) throws InterruptedException {
        // Completion service - pomoc przy leniwym podejściu
        ExecutorCompletionService<Boolean> completionService = new ExecutorCompletionService<>(cachedThreadPool);

        // lista przyszłości przydatna przy kończeniu
        ArrayList<Future<Boolean>> futuresList = new ArrayList<>();

        for (CircuitNode arg : args) {
            Future<Boolean> future =  completionService.submit(() -> parallelSolve(arg));
            futuresList.add(future);
        }

        for (int i = 0; i < args.length; i++) {
            if (Thread.currentThread().isInterrupted()) {
                cancelFutures(futuresList);
                Thread.currentThread().interrupt();
                throw new InterruptedException();
            }

            try {
                Future<Boolean> currentFuture = completionService.take();

                if (!currentFuture.get()) {
                    cancelFutures(futuresList);
                    return false;
                }
            }
            catch (InterruptedException | ExecutionException e) {
                cancelFutures(futuresList);
                Thread.currentThread().interrupt();
                throw new InterruptedException();
            }
        }
        return true;
    }

    private void cancelFutures(ArrayList<Future<Boolean>> futuresList) {
        for (Future<Boolean> future : futuresList) {
            future.cancel(true);
        }
    }
}
