// eslint-disable-next-line no-relative-import-paths/no-relative-import-paths
import { ChessEngine } from '../ChessEngineNative'

async function runPerformanceTest() {
  const engine = new ChessEngine()
  engine.newGame() // Start from default position

  const aiVersion = 'v2' // Using v2 as it has optimizations

  console.log('\n=== ITERATIVE DEEPENING PERFORMANCE TEST ===')
  console.log(
    'Position: Starting position (rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1)',
  )
  console.log('AI Version: v2')
  console.log('')

  // Test 1: Iterative Deepening (depths 1-5)
  console.log('TEST 1: Iterative Deepening (depths 1 → 5)')
  console.log('-------------------------------------------')

  const iterativeStartTime = Date.now()
  let iterativeMove = ''

  for (let depth = 1; depth <= 5; depth++) {
    const depthStartTime = Date.now()
    const move = await engine.getBestMoveAtDepth(depth, 0, aiVersion)
    const depthTime = Date.now() - depthStartTime

    console.log(`  Depth ${depth}: ${move} (${depthTime}ms)`)

    if (depth === 5) {
      iterativeMove = move
    }
  }

  const iterativeTotalTime = Date.now() - iterativeStartTime
  console.log(`  Total time: ${iterativeTotalTime}ms`)
  console.log('')

  // Reset engine to same position for fair comparison
  engine.newGame()

  // Test 2: Direct Depth 5 Search (no iterative deepening)
  console.log('TEST 2: Direct Depth 5 Search (no iterative deepening)')
  console.log('-------------------------------------------------------')

  const directStartTime = Date.now()
  const directMove = await engine.getBestMoveAtDepth(5, 0, aiVersion)
  const directTotalTime = Date.now() - directStartTime

  console.log(`  Depth 5: ${directMove} (${directTotalTime}ms)`)
  console.log('')

  // Analysis
  console.log('=== ANALYSIS ===')
  console.log(`Iterative Move: ${iterativeMove}`)
  console.log(`Direct Move: ${directMove}`)
  console.log(`Iterative Total Time: ${iterativeTotalTime}ms`)
  console.log(`Direct Total Time: ${directTotalTime}ms`)

  const difference = directTotalTime - iterativeTotalTime
  const percentage = ((Math.abs(difference) / directTotalTime) * 100).toFixed(1)

  if (iterativeTotalTime < directTotalTime) {
    const speedup = (directTotalTime / iterativeTotalTime).toFixed(2)
    console.log(
      `✅ Iterative deepening is FASTER by ${Math.abs(difference)}ms (${percentage}% faster, ${speedup}x speedup)`,
    )
    console.log(
      '   Reason: Transposition table benefits outweigh the overhead of searching shallower depths',
    )
  } else if (iterativeTotalTime > directTotalTime) {
    console.log(
      `❌ Iterative deepening is SLOWER by ${Math.abs(difference)}ms (${percentage}% slower)`,
    )
    console.log('   Reason: Overhead of multiple searches exceeds transposition table benefits')
  } else {
    console.log(`⚖️  Both approaches took the same time`)
  }
  console.log('')

  console.log('Note: For optimal performance, iterative deepening should be faster due to:')
  console.log('  1. Transposition table hits from shallower depths')
  console.log('  2. Better move ordering using previous depth results')
  console.log('  3. Aspiration windows (v2 only) using previous scores')
  console.log('')
}

// Run the test
runPerformanceTest().catch(console.error)
