import { StyleSheet, Text, View } from 'react-native'

import { theme } from '@/constants/theme'

export default function SandboxScreen() {
  return (
    <View style={styles.container}>
      <Text style={styles.title}>Sandbox</Text>
    </View>
  )
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
    backgroundColor: theme.colors.background.light,
  },
  title: {
    fontSize: theme.fontSize.xl,
    fontWeight: 'bold',
    color: theme.colors.text.light,
  },
})
