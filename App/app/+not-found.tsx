import { Link, Stack } from 'expo-router'
import { StyleSheet, Text, View } from 'react-native'

import { theme } from '@/constants/theme'

export default function NotFoundScreen() {
  return (
    <>
      <Stack.Screen options={{ title: 'Oops!' }} />
      <View style={styles.container}>
        <Text style={styles.title}>This screen doesn't exist.</Text>

        <Link href="/(tabs)/home" style={styles.link}>
          <Text style={styles.linkText}>Go to home screen!</Text>
        </Link>
      </View>
    </>
  )
}

const styles = StyleSheet.create({
  container: {
    flex: 1,
    alignItems: 'center',
    justifyContent: 'center',
    padding: theme.spacing.lg,
    backgroundColor: theme.colors.background.light,
  },
  title: {
    fontSize: theme.fontSize.lg,
    fontWeight: 'bold',
    color: theme.colors.text.light,
  },
  link: {
    marginTop: theme.spacing.md,
    paddingVertical: theme.spacing.md,
  },
  linkText: {
    fontSize: theme.fontSize.sm,
    color: theme.colors.primary,
  },
})
